#ifndef C_CHUNKER_THREAD_H_
#define C_CHUNKER_THREAD_H_

namespace chunker {
  // wrapper for async thread behavior - passed to generator
  class ChunkerThread {
    // yields the current thread to allow other threads to run
    virtual void Yield() = 0;
  };
}

#endif // C_CHUNKER_THREAD_H_
