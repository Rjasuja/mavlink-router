/*
 * This file is part of the MAVLink Router project
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "autolog.h"

#include "binlog.h"
#include "log.h"
#include "ulog.h"

int AutoLog::write_msg(const struct buffer *buffer)
{
    if (_logger != nullptr) {
        return _logger->write_msg(buffer);
    }

    const bool mavlink2 = buffer->data[0] == MAVLINK_STX;
    uint32_t msg_id;
    uint8_t *payload;

    if (mavlink2) {
        struct mavlink_router_mavlink2_header *hdr
            = (struct mavlink_router_mavlink2_header *)buffer->data;
        msg_id = hdr->msgid;
        payload = buffer->data + sizeof(*hdr);
    } else {
        struct mavlink_router_mavlink1_header *hdr
            = (struct mavlink_router_mavlink1_header *)buffer->data;
        msg_id = hdr->msgid;
        payload = buffer->data + sizeof(*hdr);
    }

    /* We check autopilot on heartbeat */
    if (msg_id == MAVLINK_MSG_ID_HEARTBEAT) {
        uint8_t autopilot = payload[5];
        log_debug("Got autopilot %u from heartbeat", autopilot);
        if (autopilot == MAV_AUTOPILOT_PX4) {
            _logger = std::unique_ptr<LogEndpoint>(new ULog(_logs_dir));
            _logger->start();
        } else if (autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            _logger = std::unique_ptr<LogEndpoint>(new BinLog(_logs_dir));
            _logger->start();
        } else {
            log_warning("Unidentified autopilot, cannot start flight stack logging");
        }
    }

    return buffer->len;
}

void AutoLog::stop()
{
    if (_logger)
        _logger->stop();

    _system_id = 0;
}

bool AutoLog::start()
{
    _system_id = LOG_ENDPOINT_SYSTEM_ID;

    return true;
}
