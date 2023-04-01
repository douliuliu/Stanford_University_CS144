#include "receiver_harness.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include<string.h>
using namespace std;

int main() {
    try {
        auto rd = get_random_generator();
        /* {
            TCPReceiver receiver =TCPReceiver(10);
            TCPSegment seg = TCPSegment();
            seg.header().syn = true;
            seg.header().seqno = WrappingInt32(0);
            receiver.segment_received(seg);

            if(receiver.ackno() != WrappingInt32(1)){
                return EXIT_FAILURE;
            }

            seg = TCPSegment();
            seg.header().seqno = WrappingInt32(1);
            seg.payload() = string("hi");
            receiver.segment_received(seg);
            // 0-syn,  1-h,  2-i
            if(receiver.ackno() != WrappingInt32(3)){
                return EXIT_FAILURE;
            }
            if(receiver.window_size() != 8){
                return EXIT_FAILURE;
            }

            seg = TCPSegment();
            seg.header().seqno = WrappingInt32(5);
            seg.payload() = string("abc");
            receiver.segment_received(seg);
            if(receiver.ackno() != WrappingInt32(3)){
                printf("receiver.ackno(): %d\n", receiver.ackno().value().raw_value());
                return EXIT_FAILURE;
            }
            if(receiver.window_size() != 8){
                printf("receiver.window_size(): %zu\n", receiver.window_size());
                return EXIT_FAILURE;
            }


            seg = TCPSegment();
            seg.header().seqno = WrappingInt32(3);
            seg.payload() = string("--");
            receiver.segment_received(seg);
            if(receiver.ackno() != WrappingInt32(8)){
                printf("receiver.ackno(): %d\n", receiver.ackno().value().raw_value());
                return EXIT_FAILURE;
            }
            if(receiver.window_size() != 3){
                printf("receiver.window_size(): %zu\n", receiver.window_size());
                return EXIT_FAILURE;
            }

            string s = receiver.stream_out().read(8);
            if(strcmp(s.c_str(), "hi--abc" ) != 0){
                printf("s: %s\n", s.c_str());
                return EXIT_FAILURE;
            }
            if(receiver.window_size() != 10){
                printf("receiver.window_size() != 10\n");
                return EXIT_FAILURE;
            }
        } */

        {
            uint32_t isn = uniform_int_distribution<uint32_t>{0, UINT32_MAX}(rd);
            TCPReceiverTestHarness test{4000};
            test.execute(ExpectState{TCPReceiverStateSummary::LISTEN});
            test.execute(SegmentArrives{}.with_syn().with_seqno(isn + 0).with_result(SegmentArrives::Result::OK));
            test.execute(ExpectState{TCPReceiverStateSummary::SYN_RECV});
            test.execute(SegmentArrives{}.with_fin().with_seqno(isn + 1).with_result(SegmentArrives::Result::OK));
            test.execute(ExpectAckno{WrappingInt32{isn + 2}});
            test.execute(ExpectUnassembledBytes{0});
            test.execute(ExpectBytes{""});
            test.execute(ExpectTotalAssembledBytes{0});
            test.execute(ExpectState{TCPReceiverStateSummary::FIN_RECV});
        }

        {
            uint32_t isn = uniform_int_distribution<uint32_t>{0, UINT32_MAX}(rd);
            TCPReceiverTestHarness test{4000};
            test.execute(ExpectState{TCPReceiverStateSummary::LISTEN});
            test.execute(SegmentArrives{}.with_syn().with_seqno(isn + 0).with_result(SegmentArrives::Result::OK));
            test.execute(ExpectState{TCPReceiverStateSummary::SYN_RECV});
            test.execute(
                SegmentArrives{}.with_fin().with_seqno(isn + 1).with_data("a").with_result(SegmentArrives::Result::OK));
            test.execute(ExpectState{TCPReceiverStateSummary::FIN_RECV});
            test.execute(ExpectAckno{WrappingInt32{isn + 3}});
            test.execute(ExpectUnassembledBytes{0});
            test.execute(ExpectBytes{"a"});
            test.execute(ExpectTotalAssembledBytes{1});
            test.execute(ExpectState{TCPReceiverStateSummary::FIN_RECV});
        }

    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
