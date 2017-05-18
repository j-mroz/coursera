namespace cpp proto.dht

const byte PROTOCOL_MAGIC   = 0xD5
const byte PROTOCOL_VERSION = 0x01

enum ReqType {
    CREATE,
    READ,
    UPDATE,
    DELETE,
    CREATE_RSP,
    READ_RSP,
    DELETE_RSP,
    UPDATE_RSP,
    SYNC_BEGIN,
    SYNC_END
}

enum ReqStatus {
    OK   = 0,
    FAIL = 1
}

struct IpAddr {
    1: binary bytes
}

struct Header {
    1: byte         protocol = PROTOCOL_MAGIC,
    2: byte         version  = PROTOCOL_VERSION,
    3: ReqType      type,
    4: ReqStatus    status,
    5: i32          seqId,
    6: i32          transaction,
    7: IpAddr       srcAddr,
    8: i16          srcPort
}

struct Body {
    1: string key,
    2: string value,
    3: map<string, string> keyValueMap
}

struct Message {
    1: Header header,
    3: Body   body
}
