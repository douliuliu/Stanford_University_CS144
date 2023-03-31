#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.

void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t data_len = data.size(); //数据的长度
    string valid_data = data;
    bool _eof = eof;
    size_t idx = index; //原来的index是常量无法改变，重新赋给一个idx
    if(idx < _first_unassembled){
        if(_first_unassembled - idx >= data_len){
            idx = _first_unassembled;
            data_len = 0;
            valid_data = "";
        }
        else
        {
            valid_data = valid_data.substr(_first_unassembled - idx);
            // cout<<valid_data<<endl;
            idx = _first_unassembled;
            data_len = valid_data.size();
        }
    }

    if(idx + data_len - _first_unassembled > _capacity - _output.buffer_size()){
        data_len = ( _first_unassembled + (_capacity - _output.buffer_size()) ) - idx ;
        valid_data = valid_data.substr(0, data_len);
        _eof = false;   // 如何进入该if条件中，必然会把最后一个字节截取掉，故就算eof为true也需要设置为
    }

    if(_eof){
        _eof_flag = true;
        _end_index = idx + data_len;
    }

    //下面开始无脑往暂存区写东西
    for(size_t i = idx; i < idx + valid_data.size(); i++){
        _unassembled_bytes_map[i] = valid_data[i - idx];
    }
    string str = "";
    while (_unassembled_bytes_map.find(_first_unassembled) != _unassembled_bytes_map.end())
    {
        str += _unassembled_bytes_map[_first_unassembled];
        _unassembled_bytes_map.erase(_first_unassembled);
        _first_unassembled++;
    }
    if(str.size() > 0){
    _output.write(str);
    }
    if(_eof_flag && _first_unassembled == _end_index){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return _unassembled_bytes_map.size();
}

bool StreamReassembler::empty() const { return _unassembled_bytes_map.size() == 0; }
