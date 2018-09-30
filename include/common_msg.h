#ifndef INCLUDE_COMMON_MSG_H
#define INCLUDE_COMMON_MSG_H

#define MSG_MAX_LENGTH 1024

/* All messages start with this byte */
#define MSG_ID 0xAA

enum msg_cmd_id {
    MSG_CMD_READ,
    MSG_CMD_WRITE,
    MSG_CMD_ERASE_SECTOR,
    MSG_CMD_ERASE_CHIP,
};

#define MSG_RESPONSE_INFO_X \
    X(MSG_SUCCESS,                        "Command completed sucessfully.") \
    X(MSG_SUCCESS_PING,                   "Null command was received sucessfully.") \
    X(MSG_ERROR_INVALID_ID,               "Invalid message ID.") \
    X(MSG_ERROR_OVERFLOW,                 "Command overflowed buffer on programmer.") \
    X(MSG_ERROR_INVALID_CMD,              "Invalid command ID.") \
    X(MSG_ERROR_READ_INVALID_MSG,         "Invalid read command.") \
    X(MSG_ERROR_WRITE_INVALID_MSG,        "Invalid write command.") \
    X(MSG_ERROR_WRITE_INVALID_DATA,       "Invalid data to write.") \
    X(MSG_ERROR_WRITE_FAILURE,            "Write to the EEPROM failed.") \
    X(MSG_ERROR_ERASE_SECTOR_INVALID_MSG, "Invalid erase sector command.") \
    X(MSG_ERROR_ERASE_SECTOR_FAILED,      "EEPROM failed to erase sector.") \


enum msg_cmd_response {
#define X(enm, ...) enm,
MSG_RESPONSE_INFO_X
#undef X
};

#endif
