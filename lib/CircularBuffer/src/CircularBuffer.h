#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER

#include <Arduino.h>

template <class T>
class CircularBuffer
{
private:
  T *buffer;
  void (*deallocator)(T);
  int start, end, dim;
  SemaphoreHandle_t mutex, libero, contenuto;

public:
  CircularBuffer() : start(0), end(0), dim(10)
  {
    this->mutex = nullptr;
    this->contenuto = nullptr;
    this->libero = nullptr;
    buffer = (T *)ps_malloc(sizeof(T) * dim);
    deallocator = nullptr;
  }

  CircularBuffer(int _dim, bool mutex = false) : start(0), end(0), dim(_dim + 1)
  {
    buffer = (T *)ps_malloc(sizeof(T) * dim);
    deallocator = nullptr;
    if (mutex)
    {
      this->mutex = xSemaphoreCreateMutex();
      this->contenuto = xSemaphoreCreateCounting(this->dim - 1, 0);
      this->libero = xSemaphoreCreateCounting(this->dim - 1, this->dim - 1);
    }
    else
    {
      this->mutex = nullptr;
      this->contenuto = nullptr;
      this->libero = nullptr;
    }
  }

  CircularBuffer(int _dim, void (*deallocator)(T), bool mutex = false) : start(0), end(0), dim(_dim + 1)
  {
    buffer = new T[dim];
    this->deallocator = deallocator;
    if (mutex)
    {
      this->mutex = xSemaphoreCreateMutex();
      this->contenuto = xSemaphoreCreateCounting(this->dim - 1, 0);
      this->libero = xSemaphoreCreateCounting(this->dim - 1, this->dim - 1);
    }
    else
    {
      this->mutex = nullptr;
      this->contenuto = nullptr;
      this->libero = nullptr;
    }
  }

  CircularBuffer(const CircularBuffer &c) = delete;

  void push(const T object)
  {
    if (mutex != nullptr)
    {
      xSemaphoreTake(libero, portMAX_DELAY);
      xSemaphoreTake(mutex, portMAX_DELAY);
    }
    buffer[end] = object;
    end = (end + 1) % dim;
    log_d("[PUSH] %d %d", start, end);
    log_d("heap: %d", ESP.getFreeHeap());
    log_d("psram: %d", ESP.getFreePsram());
    if (mutex != nullptr)
    {
      xSemaphoreGive(mutex);
      xSemaphoreGive(contenuto);
    }
  }

  T pop()
  {
    if (mutex != nullptr)
    {
      xSemaphoreTake(contenuto, portMAX_DELAY);
      xSemaphoreTake(mutex, portMAX_DELAY);
    }
    T ret = buffer[start];
    start = (start + 1) % dim;
    log_d("[POP] %d %d", start, end);
    if (mutex != nullptr)
    {
      xSemaphoreGive(mutex);
      xSemaphoreGive(libero);
    }

    return ret;
  }

  bool can_pop() const
  {
    log_d("[CAN POP]");
    if (start == end)
    {
      return false;
    }
    return true;
  }

  bool can_push() const
  {
    if (start == (end + 1) % dim)
    {
      return false;
    }
    return true;
  }

  void log() const
  {
    if (mutex != nullptr)
    {
      xSemaphoreTake(mutex, portMAX_DELAY);
    }
    log_d("[buffer] %d %d", start, end);
    if (mutex != nullptr)
    {
      xSemaphoreGive(mutex);
    }
  }

  ~CircularBuffer()
  {
    if (deallocator != nullptr)
    {
      for (; start != end; start = (start + 1) % dim)
      {
        deallocator(buffer[start]);
      }
    }
    delete[] buffer;
  }
};

#endif
