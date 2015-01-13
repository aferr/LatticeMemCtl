/*
 * Copyright (c) 1999-2012 Mark D. Hill and David A. Wood
 * Copyright (c) 2010 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "debug/RubyCacheTrace.hh"
#include "mem/ruby/recorder/CacheRecorder.hh"
#include "mem/ruby/system/Sequencer.hh"
#include "mem/ruby/system/System.hh"

using namespace std;

void
TraceRecord::print(ostream& out) const
{
    out << "[TraceRecord: Node, " << m_cntrl_id << ", "
        << m_data_address << ", " << m_pc_address << ", "
        << m_type << ", Time: " << m_time << "]";
}

CacheRecorder::CacheRecorder()
    : m_uncompressed_trace(NULL),
      m_uncompressed_trace_size(0)
{
}

CacheRecorder::CacheRecorder(uint8_t* uncompressed_trace,
                             uint64_t uncompressed_trace_size,
                             std::vector<Sequencer*>& seq_map)
    : m_uncompressed_trace(uncompressed_trace),
      m_uncompressed_trace_size(uncompressed_trace_size),
      m_seq_map(seq_map),  m_bytes_read(0), m_records_read(0),
      m_records_flushed(0)
{
}

CacheRecorder::~CacheRecorder()
{
    if (m_uncompressed_trace != NULL) {
        delete m_uncompressed_trace;
        m_uncompressed_trace = NULL;
    }
    m_seq_map.clear();
}

void
CacheRecorder::enqueueNextFlushRequest()
{
    panic("CacheRecorder::enqueueNextFushRequest\n");
    if (m_records_flushed < m_records.size()) {
        TraceRecord* rec = m_records[m_records_flushed];
        m_records_flushed++;
        Request* req = new Request(rec->m_data_address,
                                   RubySystem::getBlockSizeBytes(),0,
                                   Request::funcMasterId);
        MemCmd::Command requestType = MemCmd::FlushReq;
        Packet *pkt = new Packet(req, requestType, -1, -1, -1);

        Sequencer* m_sequencer_ptr = m_seq_map[rec->m_cntrl_id];
        assert(m_sequencer_ptr != NULL);
        m_sequencer_ptr->makeRequest(pkt);

        DPRINTF(RubyCacheTrace, "Flushing %s\n", *rec);
    }
}

void
CacheRecorder::enqueueNextFetchRequest()
{
    panic("CacheRecorder::enqueueNextFushRequest\n");
    if (m_bytes_read < m_uncompressed_trace_size) {
        TraceRecord* traceRecord = (TraceRecord*) (m_uncompressed_trace +
                                                                m_bytes_read);

        DPRINTF(RubyCacheTrace, "Issuing %s\n", *traceRecord);
        Request* req = new Request();
        MemCmd::Command requestType;

        if (traceRecord->m_type == RubyRequestType_LD) {
            requestType = MemCmd::ReadReq;
            req->setPhys(traceRecord->m_data_address,
                    RubySystem::getBlockSizeBytes(),0, Request::funcMasterId);
        }   else if (traceRecord->m_type == RubyRequestType_IFETCH) {
            requestType = MemCmd::ReadReq;
            req->setPhys(traceRecord->m_data_address,
                    RubySystem::getBlockSizeBytes(),
                    Request::INST_FETCH, Request::funcMasterId);
        }   else {
            requestType = MemCmd::WriteReq;
            req->setPhys(traceRecord->m_data_address,
                    RubySystem::getBlockSizeBytes(),0, Request::funcMasterId);
        }

        Packet *pkt = new Packet(req, requestType, -1, -1, -1);
        pkt->dataStatic(traceRecord->m_data);

        Sequencer* m_sequencer_ptr = m_seq_map[traceRecord->m_cntrl_id];
        assert(m_sequencer_ptr != NULL);
        m_sequencer_ptr->makeRequest(pkt);

        m_bytes_read += (sizeof(TraceRecord) +
                RubySystem::getBlockSizeBytes());
        m_records_read++;
    }
}

void
CacheRecorder::addRecord(int cntrl, const physical_address_t data_addr,
                         const physical_address_t pc_addr,
                         RubyRequestType type, Time time, DataBlock& data)
{
    TraceRecord* rec = (TraceRecord*)malloc(sizeof(TraceRecord) +
                                            RubySystem::getBlockSizeBytes());
    rec->m_cntrl_id     = cntrl;
    rec->m_time         = time;
    rec->m_data_address = data_addr;
    rec->m_pc_address   = pc_addr;
    rec->m_type         = type;
    memcpy(rec->m_data, data.getData(0, RubySystem::getBlockSizeBytes()),
           RubySystem::getBlockSizeBytes());

    m_records.push_back(rec);
}

uint64
CacheRecorder::aggregateRecords(uint8_t** buf, uint64 total_size)
{
    std::sort(m_records.begin(), m_records.end(), compareTraceRecords);

    int size = m_records.size();
    uint64 current_size = 0;
    int record_size = sizeof(TraceRecord) + RubySystem::getBlockSizeBytes();

    for (int i = 0; i < size; ++i) {
        // Determine if we need to expand the buffer size
        if (current_size + record_size > total_size) {
            uint8_t* new_buf = new (nothrow) uint8_t[total_size * 2];
            if (new_buf == NULL) {
                fatal("Unable to allocate buffer of size %s\n",
                      total_size * 2);
            }
            total_size = total_size * 2;
            uint8_t* old_buf = *buf;
            memcpy(new_buf, old_buf, current_size);
            *buf = new_buf;
            delete [] old_buf;
        }

        // Copy the current record into the buffer
        memcpy(&((*buf)[current_size]), m_records[i], record_size);
        current_size += record_size;

        free(m_records[i]);
        m_records[i] = NULL;
    }

    m_records.clear();
    return current_size;
}
