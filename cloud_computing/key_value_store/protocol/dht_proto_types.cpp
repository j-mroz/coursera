/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "dht_proto_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace proto { namespace dht {

int _kReqTypeValues[] = {
  ReqType::CREATE,
  ReqType::READ,
  ReqType::UPDATE,
  ReqType::DELETE,
  ReqType::CREATE_RSP,
  ReqType::READ_RSP,
  ReqType::DELETE_RSP,
  ReqType::UPDATE_RSP,
  ReqType::SYNC_BEGIN,
  ReqType::SYNC_END
};
const char* _kReqTypeNames[] = {
  "CREATE",
  "READ",
  "UPDATE",
  "DELETE",
  "CREATE_RSP",
  "READ_RSP",
  "DELETE_RSP",
  "UPDATE_RSP",
  "SYNC_BEGIN",
  "SYNC_END"
};
const std::map<int, const char*> _ReqType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(10, _kReqTypeValues, _kReqTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

int _kReqStatusValues[] = {
  ReqStatus::OK,
  ReqStatus::FAIL
};
const char* _kReqStatusNames[] = {
  "OK",
  "FAIL"
};
const std::map<int, const char*> _ReqStatus_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(2, _kReqStatusValues, _kReqStatusNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));


IpAddr::~IpAddr() throw() {
}


void IpAddr::__set_bytes(const std::string& val) {
  this->bytes = val;
}

void swap(IpAddr &a, IpAddr &b) {
  using ::std::swap;
  swap(a.bytes, b.bytes);
  swap(a.__isset, b.__isset);
}

IpAddr::IpAddr(const IpAddr& other0) {
  bytes = other0.bytes;
  __isset = other0.__isset;
}
IpAddr& IpAddr::operator=(const IpAddr& other1) {
  bytes = other1.bytes;
  __isset = other1.__isset;
  return *this;
}
void IpAddr::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "IpAddr(";
  out << "bytes=" << to_string(bytes);
  out << ")";
}


Header::~Header() throw() {
}


void Header::__set_protocol(const int8_t val) {
  this->protocol = val;
}

void Header::__set_version(const int8_t val) {
  this->version = val;
}

void Header::__set_type(const ReqType::type val) {
  this->type = val;
}

void Header::__set_status(const ReqStatus::type val) {
  this->status = val;
}

void Header::__set_seqId(const int32_t val) {
  this->seqId = val;
}

void Header::__set_transaction(const int32_t val) {
  this->transaction = val;
}

void Header::__set_srcAddr(const IpAddr& val) {
  this->srcAddr = val;
}

void Header::__set_srcPort(const int16_t val) {
  this->srcPort = val;
}

void swap(Header &a, Header &b) {
  using ::std::swap;
  swap(a.protocol, b.protocol);
  swap(a.version, b.version);
  swap(a.type, b.type);
  swap(a.status, b.status);
  swap(a.seqId, b.seqId);
  swap(a.transaction, b.transaction);
  swap(a.srcAddr, b.srcAddr);
  swap(a.srcPort, b.srcPort);
  swap(a.__isset, b.__isset);
}

Header::Header(const Header& other4) {
  protocol = other4.protocol;
  version = other4.version;
  type = other4.type;
  status = other4.status;
  seqId = other4.seqId;
  transaction = other4.transaction;
  srcAddr = other4.srcAddr;
  srcPort = other4.srcPort;
  __isset = other4.__isset;
}
Header& Header::operator=(const Header& other5) {
  protocol = other5.protocol;
  version = other5.version;
  type = other5.type;
  status = other5.status;
  seqId = other5.seqId;
  transaction = other5.transaction;
  srcAddr = other5.srcAddr;
  srcPort = other5.srcPort;
  __isset = other5.__isset;
  return *this;
}
void Header::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "Header(";
  out << "protocol=" << to_string(protocol);
  out << ", " << "version=" << to_string(version);
  out << ", " << "type=" << to_string(type);
  out << ", " << "status=" << to_string(status);
  out << ", " << "seqId=" << to_string(seqId);
  out << ", " << "transaction=" << to_string(transaction);
  out << ", " << "srcAddr=" << to_string(srcAddr);
  out << ", " << "srcPort=" << to_string(srcPort);
  out << ")";
}


Body::~Body() throw() {
}


void Body::__set_key(const std::string& val) {
  this->key = val;
}

void Body::__set_value(const std::string& val) {
  this->value = val;
}

void Body::__set_keyValueMap(const std::map<std::string, std::string> & val) {
  this->keyValueMap = val;
}

void swap(Body &a, Body &b) {
  using ::std::swap;
  swap(a.key, b.key);
  swap(a.value, b.value);
  swap(a.keyValueMap, b.keyValueMap);
  swap(a.__isset, b.__isset);
}

Body::Body(const Body& other14) {
  key = other14.key;
  value = other14.value;
  keyValueMap = other14.keyValueMap;
  __isset = other14.__isset;
}
Body& Body::operator=(const Body& other15) {
  key = other15.key;
  value = other15.value;
  keyValueMap = other15.keyValueMap;
  __isset = other15.__isset;
  return *this;
}
void Body::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "Body(";
  out << "key=" << to_string(key);
  out << ", " << "value=" << to_string(value);
  out << ", " << "keyValueMap=" << to_string(keyValueMap);
  out << ")";
}


Message::~Message() throw() {
}


void Message::__set_header(const Header& val) {
  this->header = val;
}

void Message::__set_body(const Body& val) {
  this->body = val;
}

void swap(Message &a, Message &b) {
  using ::std::swap;
  swap(a.header, b.header);
  swap(a.body, b.body);
  swap(a.__isset, b.__isset);
}

Message::Message(const Message& other16) {
  header = other16.header;
  body = other16.body;
  __isset = other16.__isset;
}
Message& Message::operator=(const Message& other17) {
  header = other17.header;
  body = other17.body;
  __isset = other17.__isset;
  return *this;
}
void Message::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "Message(";
  out << "header=" << to_string(header);
  out << ", " << "body=" << to_string(body);
  out << ")";
}

}} // namespace