/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "ss_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>




Test::~Test() throw() {
}


void Test::__set_id(const int32_t val) {
  this->id = val;
}

void Test::__set_value(const int32_t val) {
  this->value = val;
}

uint32_t Test::read(::apache::thrift::protocol::TProtocol* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->id);
          this->__isset.id = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->value);
          this->__isset.value = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t Test::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("Test");

  xfer += oprot->writeFieldBegin("id", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->id);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("value", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->value);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(Test &a, Test &b) {
  using ::std::swap;
  swap(a.id, b.id);
  swap(a.value, b.value);
  swap(a.__isset, b.__isset);
}

Test::Test(const Test& other0) {
  id = other0.id;
  value = other0.value;
  __isset = other0.__isset;
}
Test& Test::operator=(const Test& other1) {
  id = other1.id;
  value = other1.value;
  __isset = other1.__isset;
  return *this;
}
void Test::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "Test(";
  out << "id=" << to_string(id);
  out << ", " << "value=" << to_string(value);
  out << ")";
}

