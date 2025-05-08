/**
 * @file
 * Lock-free multi-producer single-consumer (MPSC) ring buffer.
 * Based on: https://github.com/rmind/ringbuf
 */

#ifndef RINGBUF_HPP
#define RINGBUF_HPP

#include <cstddef>

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <thread>

#if _MSC_VER
#include <intrin.h>
#include <windows.h>
#endif

#ifdef _WIN32
#define __predict_false(x) (x)
#else
#define __predict_false(x) __builtin_expect((x) != 0, 0)
#endif /* _WIN32 */

class atomic_backoff {
  /**
   * Time delay, in units of "pause" instructions.
   * Should be equal to approximately the number of "pause" instructions
   * that take the same time as context switch. Must be a power of two.
   */
  static const int32_t LOOPS_BEFORE_YIELD = 16;
  int32_t count;

  static inline void __pause(int32_t delay) {
    for (; delay > 0; --delay) {
#if _MSC_VER
      YieldProcessor();
#elif __GNUC__ && (__i386__ || __x86_64__)
      // Only i386 and x86-64 have pause instruction
      __builtin_ia32_pause();
#endif
    }
  }

public:
  /**
   * Deny copy constructor
   */
  atomic_backoff(const atomic_backoff &) = delete;
  /**
   * Deny assignment
   */
  atomic_backoff &operator=(const atomic_backoff &) = delete;

  /** Default constructor */
  /* In many cases, an object of this type is initialized eagerly on hot
   * path, as in for(atomic_backoff b; ; b.pause()) {...} For this reason,
   * the construction cost must be very small! */
  atomic_backoff() : count(1) {}

  /**
   * This constructor pauses immediately; do not use on hot paths!
   */
  atomic_backoff(bool) : count(1) { pause(); }

  /**
   * Pause for a while.
   */
  void pause() {
    if (count <= LOOPS_BEFORE_YIELD) {
      __pause(count);
      /* Pause twice as long the next time. */
      count *= 2;
    } else {
      /* Pause is so long that we might as well yield CPU to
       * scheduler. */
      std::this_thread::yield();
    }
  }

  /**
   * Pause for a few times and return false if saturated.
   */
  bool bounded_pause() {
    __pause(count);
    if (count < LOOPS_BEFORE_YIELD) {
      /* Pause twice as long the next time. */
      count *= 2;
      return true;
    } else {
      return false;
    }
  }

  void reset() { count = 1; }
}; /* class atomic_backoff */

static constexpr size_t RBUF_OFF_MASK = 0x00000000ffffffffUL;
static constexpr size_t WRAP_LOCK_BIT = 0x8000000000000000UL;
static constexpr size_t RBUF_OFF_MAX = UINT64_MAX & ~WRAP_LOCK_BIT;

static constexpr size_t WRAP_COUNTER = 0x7fffffff00000000UL;
static size_t WRAP_INCR(size_t x) {
  return ((x + 0x100000000UL) & WRAP_COUNTER);
}

typedef uint64_t ringbuf_off_t;

struct ringbuf_worker_t {
  std::atomic<ringbuf_off_t> seen_off{0};
  std::atomic<int> registered{0};
};

struct ringbuf_t {
  /* Ring buffer space. */
  size_t space;

  /*
   * The NEXT hand is atomically updated by the producer.
   * WRAP_LOCK_BIT is set in case of wrap-around; in such case,
   * the producer can update the 'end' offset.
   */
  std::atomic<ringbuf_off_t> next{0};
  std::atomic<ringbuf_off_t> end{0};

  /* The following are updated by the consumer. */
  std::atomic<ringbuf_off_t> written{0};
  unsigned nworkers;
  std::unique_ptr<ringbuf_worker_t[]> workers;

  /* Set by ringbuf_consume, reset by ringbuf_release. */
  bool consume_in_progress;

  /**
   * Creates new ringbuf_t instance.
   *
   * Length must be < RBUF_OFF_MASK
   */
  ringbuf_t(size_t max_workers, size_t length)
      : workers(new ringbuf_worker_t[max_workers]) {
    if (length >= RBUF_OFF_MASK)
      throw std::out_of_range("ringbuf length too big");

    space = length;
    end = RBUF_OFF_MAX;
    nworkers = max_workers;
    consume_in_progress = false;

    /* Helgrind/Drd does not understand std::atomic */
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
    VALGRIND_HG_DISABLE_CHECKING(&next, sizeof(next));
    VALGRIND_HG_DISABLE_CHECKING(&end, sizeof(end));
    VALGRIND_HG_DISABLE_CHECKING(&written, sizeof(written));

    for (size_t i = 0; i < max_workers; i++) {
      VALGRIND_HG_DISABLE_CHECKING(&workers[i].seen_off,
                                   sizeof(workers[i].seen_off));
      VALGRIND_HG_DISABLE_CHECKING(&workers[i].registered,
                                   sizeof(workers[i].registered));
    }
#endif
  }
};

/*
 * ringbuf_register: register the worker (thread/process) as a producer
 * and pass the pointer to its local store.
 */
inline ringbuf_worker_t *ringbuf_register(ringbuf_t *rbuf, unsigned i) {
  ringbuf_worker_t *w = &rbuf->workers[i];

  w->seen_off = RBUF_OFF_MAX;
  std::atomic_store_explicit<int>(&w->registered, true,
                                  std::memory_order_release);
  return w;
}

inline void ringbuf_unregister(ringbuf_t *rbuf, ringbuf_worker_t *w) {
  w->registered = false;
  (void)rbuf;
}

/*
 * stable_nextoff: capture and return a stable value of the 'next' offset.
 */
static inline ringbuf_off_t stable_nextoff(ringbuf_t *rbuf) {
  ringbuf_off_t next;
  for (atomic_backoff backoff;;) {
    next = std::atomic_load_explicit<ringbuf_off_t>(&rbuf->next,
                                                    std::memory_order_acquire);
    if (next & WRAP_LOCK_BIT) {
      backoff.pause();
    } else {
      break;
    }
  }
  assert((next & RBUF_OFF_MASK) < rbuf->space);
  return next;
}

/*
 * stable_seenoff: capture and return a stable value of the 'seen' offset.
 */
static inline ringbuf_off_t stable_seenoff(ringbuf_worker_t *w) {
  ringbuf_off_t seen_off;
  for (atomic_backoff backoff;;) {
    seen_off = std::atomic_load_explicit<ringbuf_off_t>(
        &w->seen_off, std::memory_order_acquire);
    if (seen_off & WRAP_LOCK_BIT) {
      backoff.pause();
    } else {
      break;
    }
  }
  return seen_off;
}

/*
 * ringbuf_acquire: request a space of a given length in the ring buffer.
 *
 * => On success: returns the offset at which the space is available.
 * => On failure: returns -1.
 */
inline ptrdiff_t ringbuf_acquire(ringbuf_t *rbuf, ringbuf_worker_t *w,
                                 size_t len) {
  ringbuf_off_t seen, next, target;

  assert(len > 0 && len <= rbuf->space);
  assert(w->seen_off == RBUF_OFF_MAX);

  do {
    ringbuf_off_t written;

    /*
     * Get the stable 'next' offset.  Save the observed 'next'
     * value (i.e. the 'seen' offset), but mark the value as
     * unstable (set WRAP_LOCK_BIT).
     *
     * Note: CAS will issue a memory_order_release for us and
     * thus ensures that it reaches global visibility together
     * with new 'next'.
     */
    seen = stable_nextoff(rbuf);
    next = seen & RBUF_OFF_MASK;
    assert(next < rbuf->space);
    std::atomic_store_explicit<ringbuf_off_t>(
        &w->seen_off, next | WRAP_LOCK_BIT, std::memory_order_relaxed);

    /*
     * Compute the target offset.  Key invariant: we cannot
     * go beyond the WRITTEN offset or catch up with it.
     */
    target = next + len;
    written = rbuf->written;
    if (__predict_false(next < written && target >= written)) {
      /* The producer must wait. */
      std::atomic_store_explicit<ringbuf_off_t>(&w->seen_off, RBUF_OFF_MAX,
                                                std::memory_order_release);
      return -1;
    }

    if (__predict_false(target >= rbuf->space)) {
      const bool exceed = target > rbuf->space;

      /*
       * Wrap-around and start from the beginning.
       *
       * If we would exceed the buffer, then attempt to
       * acquire the WRAP_LOCK_BIT and use the space in
       * the beginning.  If we used all space exactly to
       * the end, then reset to 0.
       *
       * Check the invariant again.
       */
      target = exceed ? (WRAP_LOCK_BIT | len) : 0;
      if ((target & RBUF_OFF_MASK) >= written) {
        std::atomic_store_explicit<ringbuf_off_t>(&w->seen_off, RBUF_OFF_MAX,
                                                  std::memory_order_release);
        return -1;
      }
      /* Increment the wrap-around counter. */
      target |= WRAP_INCR(seen & WRAP_COUNTER);
    } else {
      /* Preserve the wrap-around counter. */
      target |= seen & WRAP_COUNTER;
    }
  } while (!std::atomic_compare_exchange_weak<ringbuf_off_t>(&rbuf->next, &seen,
                                                             target));

  /*
   * Acquired the range.  Clear WRAP_LOCK_BIT in the 'seen' value
   * thus indicating that it is stable now.
   *
   * No need for memory_order_release, since CAS issued a fence.
   */
  std::atomic_store_explicit<ringbuf_off_t>(
      &w->seen_off, w->seen_off & ~WRAP_LOCK_BIT, std::memory_order_relaxed);

  /*
   * If we set the WRAP_LOCK_BIT in the 'next' (because we exceed
   * the remaining space and need to wrap-around), then save the
   * 'end' offset and release the lock.
   */
  if (__predict_false(target & WRAP_LOCK_BIT)) {
    /* Cannot wrap-around again if consumer did not catch-up. */
    assert(rbuf->written <= next);
    assert(rbuf->end == RBUF_OFF_MAX);
    rbuf->end = next;
    next = 0;

    /*
     * Unlock: ensure the 'end' offset reaches global
     * visibility before the lock is released.
     */
    std::atomic_store_explicit<ringbuf_off_t>(
        &rbuf->next, (target & ~WRAP_LOCK_BIT), std::memory_order_release);
  }
  assert((target & RBUF_OFF_MASK) <= rbuf->space);
  return (ptrdiff_t)next;
}

/*
 * ringbuf_produce: indicate the acquired range in the buffer is produced
 * and is ready to be consumed.
 */
inline void ringbuf_produce(ringbuf_t *rbuf, ringbuf_worker_t *w) {
  (void)rbuf;
  assert(w->registered);
  assert(w->seen_off != RBUF_OFF_MAX);
  std::atomic_store_explicit<ringbuf_off_t>(&w->seen_off, RBUF_OFF_MAX,
                                            std::memory_order_release);
}

/*
 * ringbuf_consume: get a contiguous range which is ready to be consumed.
 *
 * Nested consumes are not allowed.
 */
inline size_t ringbuf_consume(ringbuf_t *rbuf, size_t *offset) {
  assert(!rbuf->consume_in_progress);

  ringbuf_off_t written = rbuf->written, next, ready;
  size_t towrite;
retry:
  /*
   * Get the stable 'next' offset.  Note: stable_nextoff() issued
   * a load memory barrier.  The area between the 'written' offset
   * and the 'next' offset will be the *preliminary* target buffer
   * area to be consumed.
   */
  next = stable_nextoff(rbuf) & RBUF_OFF_MASK;
  if (written == next) {
    /* If producers did not advance, then nothing to do. */
    return 0;
  }

  /*
   * Observe the 'ready' offset of each producer.
   *
   * At this point, some producer might have already triggered the
   * wrap-around and some (or all) seen 'ready' values might be in
   * the range between 0 and 'written'.  We have to skip them.
   */
  ready = RBUF_OFF_MAX;

  for (unsigned i = 0; i < rbuf->nworkers; i++) {
    ringbuf_worker_t *w = &rbuf->workers[i];
    ringbuf_off_t seen_off;

    /*
     * Skip if the worker has not registered.
     *
     * Get a stable 'seen' value.  This is necessary since we
     * want to discard the stale 'seen' values.
     */
    if (!std::atomic_load_explicit<int>(&w->registered,
                                        std::memory_order_relaxed))
      continue;
    seen_off = stable_seenoff(w);

    /*
     * Ignore the offsets after the possible wrap-around.
     * We are interested in the smallest seen offset that is
     * not behind the 'written' offset.
     */
    if (seen_off >= written) {
      ready = std::min<ringbuf_off_t>(seen_off, ready);
    }
    assert(ready >= written);
  }

  /*
   * Finally, we need to determine whether wrap-around occurred
   * and deduct the safe 'ready' offset.
   */
  if (next < written) {
    const ringbuf_off_t end = std::min<ringbuf_off_t>(rbuf->space, rbuf->end);

    /*
     * Wrap-around case.  Check for the cut off first.
     *
     * Reset the 'written' offset if it reached the end of
     * the buffer or the 'end' offset (if set by a producer).
     * However, we must check that the producer is actually
     * done (the observed 'ready' offsets are clear).
     */
    if (ready == RBUF_OFF_MAX && written == end) {
      /*
       * Clear the 'end' offset if was set.
       */
      if (rbuf->end != RBUF_OFF_MAX) {
        rbuf->end = RBUF_OFF_MAX;
      }

      /*
       * Wrap-around the consumer and start from zero.
       */
      written = 0;
      std::atomic_store_explicit<ringbuf_off_t>(&rbuf->written, written,
                                                std::memory_order_release);
      goto retry;
    }

    /*
     * We cannot wrap-around yet; there is data to consume at
     * the end.  The ready range is smallest of the observed
     * 'ready' or the 'end' offset.  If neither is set, then
     * the actual end of the buffer.
     */
    assert(ready > next);
    ready = std::min<ringbuf_off_t>(ready, end);
    assert(ready >= written);
  } else {
    /*
     * Regular case.  Up to the observed 'ready' (if set)
     * or the 'next' offset.
     */
    ready = std::min<ringbuf_off_t>(ready, next);
  }
  towrite = ready - written;
  *offset = written;

  assert(ready >= written);
  assert(towrite <= rbuf->space);

  if (towrite)
    rbuf->consume_in_progress = true;

  return towrite;
}

/*
 * ringbuf_release: indicate that the consumed range can now be released.
 */
inline void ringbuf_release(ringbuf_t *rbuf, size_t nbytes) {
  rbuf->consume_in_progress = false;

  const size_t nwritten = rbuf->written + nbytes;

  assert(rbuf->written <= rbuf->space);
  assert(rbuf->written <= rbuf->end);
  assert(nwritten <= rbuf->space);

  rbuf->written = (nwritten == rbuf->space) ? 0 : nwritten;
}

#endif
