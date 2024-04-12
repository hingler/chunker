#ifndef CHUNKER_THREAD_HANDLE_H_
#define CHUNKER_THREAD_HANDLE_H_

//
namespace chunker {
  class ThreadHandle {
   public:
    // frees this handle manually.
    virtual void Release() = 0;

    //
    virtual ~ThreadHandle() {}
  };
}

#endif // CHUNKER_THREAD_HANDLE_H_
