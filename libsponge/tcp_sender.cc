#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

#include <assert.h>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {
        _retransmission_timeout = retx_timeout;
    }

// 已发送但还没有收到确认的字节数
uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _abs_ackno; }

void TCPSender::fill_window() {
    if(_fin_sent){
        return;
    }
    while(bytes_in_flight() < _window_size){ // 我们认为对方接收窗口还没满，可以继续发对方窗口范围内可接收数据
        TCPSegment seg = TCPSegment();
        if(!_syn_sent){
            seg.header().syn = true;
            seg.header().seqno = _isn;
            _syn_sent = true;
        }
        size_t len = min(_window_size - bytes_in_flight() - seg.length_in_sequence_space(), TCPConfig::MAX_PAYLOAD_SIZE);
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.payload() = _stream.read(len);
        if(!_fin_sent && _stream.eof()){
            // TCPConfig::MAX_PAYLOAD_SIZE only limits payload
            // Don't add fin if this will make the segment exceed the receiver's window
            if(bytes_in_flight() + seg.length_in_sequence_space() < _window_size){
                seg.header().fin = true;
                _fin_sent = true;
            }
        }
        if(seg.length_in_sequence_space() > 0){
            _segment_vector.push_back(seg);
            _segments_out.push(seg);
            _next_seqno += seg.length_in_sequence_space(); 
        }else{
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
int TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if(!_syn_sent){
        return -1;
    }
    if(window_size == 0){
        _window_size = 1;
        _zero_window = true;
    }else{
        _window_size = window_size;
        _zero_window = false;
    }
    uint64_t abs_ackno = unwrap(ackno, _isn, _abs_ackno);
    if(abs_ackno > _abs_ackno && abs_ackno <= _next_seqno){
        _ackno = ackno;
        _abs_ackno = abs_ackno;
    }else{
        return 0;
    }
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    _ticks = 0;
    if(!_syn_received && _syn_sent && _abs_ackno > 0){
        _syn_received = true;
    }
    if(!_fin_received && _fin_sent && _stream.input_ended() && _abs_ackno >= _stream.bytes_read() + 2){
        _fin_received = true;
        _segment_vector.clear();
    }
    vector<TCPSegment> segs = vector<TCPSegment>();
    for(const auto &seg : _segment_vector){
        uint64_t seqno = unwrap(seg.header().seqno, _isn, _abs_ackno);
        if(seqno + seg.length_in_sequence_space() > _abs_ackno){
            segs.push_back(seg);
        }
    }
    _segment_vector = segs;
    return 0;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(bytes_in_flight() == 0){
        if(!_segment_vector.empty()){
            printf("!_segment_vector.empty()\n");
            assert(0);
        }
        _ticks = 0;
        return;
    }
    if(_segment_vector.empty()){
        printf("_segment_vector.empty()\n");
        assert(0);
    }
    _ticks += ms_since_last_tick;
    if(_ticks >= _retransmission_timeout){
        _ticks = 0;
        _segments_out.push(_segment_vector[0]);
        if(!_zero_window){
            _retransmission_timeout *= 2;
        }
        _consecutive_retransmissions++;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg = TCPSegment();
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
