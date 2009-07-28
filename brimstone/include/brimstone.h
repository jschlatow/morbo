// -*- Mode: C++ -*-

#pragma once

#include <cstdint>
#include <iostream>
#include <exception>

// Forward declarations
struct fw_cdev_event_bus_reset;

namespace Brimstone {

  using namespace std;

  // Exception handling

  class BrimstoneException : public exception {
  };

  class SyscallError : public BrimstoneException {
  protected:
    int _errno;
  public:
    virtual const char *what() const throw();
    SyscallError() throw();
  };

  class TooManyRetriesError : public BrimstoneException {
  public:
    virtual const char *what() const throw() {
      return "Too many retries.";
    }
  };

  // I/O API

  class Node {
  protected:
    static const unsigned buffer_size = 4096;
    char _buffer[buffer_size] __attribute__((aligned(4)));

    int _fd;
    ostream &_log;

    uint32_t _node_id;
    uint32_t _local_node_id;
    uint32_t _bm_node_id;
    uint32_t _irm_node_id;
    uint32_t _root_node_id;
    uint32_t _generation;

    unsigned _in_flight;

    void update_bus_info(fw_cdev_event_bus_reset *bus_reset_event);
    void get_info();
    bool wait(bool block);
    void read_event();

  public:

    // Start an asynchronous read transaction.
    void post_read(uint64_t addr, void *buf, size_t buf_size);

    // Start an asynchronous write transaction.
    void post_write(uint64_t addr, void *buf, size_t buf_size);

    // Poll for events without blocking.
    void poll();

    // Block until all asynchronous transactions have finished.
    void barrier();

    // Synchronously read a quadlet.
    uint32_t quadlet_read(uint64_t addr) {
      uint32_t res;
      post_read(addr, &res, sizeof(res));
      barrier();
      return res;
    }

    // Synchronously write a quadlet.
    void quadlet_write(uint64_t addr, uint32_t data) {
      post_write(addr, &data, sizeof(data));
      // Do we want a barrier here?
      barrier();
    }

    int fd() const { return _fd; }
    ostream &log() const { return _log; }

    // Returns our current node ID.
    uint32_t node_id() const { return _node_id; }

    // Returns the node ID of the bus master.
    uint32_t bm_node_id() const { return _bm_node_id; }

    // Returns the node ID of the IRM.
    uint32_t irm_node_id() const { return _irm_node_id; }

    // Returns the node ID of the root node. (Should be the same as
    // the bus master?)
    uint32_t root_node_id() const { return _root_node_id; }

    // Return the current bus generation. Incremented on each bus
    // reset.
    uint32_t generation() const { return _generation; }

    // Given a path to a firewire device, such as /dev/fw1, returns an
    // object that can be used to perform memory transactions on the
    // corresponding remote node.
    Node(const char *dev, ostream &log = cout);

    ~Node();
  };

}

// EOF
