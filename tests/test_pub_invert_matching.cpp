/*
    Copyright (c) 2007-2016 Contributors as noted in the AUTHORS file

    This file is part of libzmq, the ZeroMQ core engine in C++.

    libzmq is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, the Contributors give you permission to link
    this library with independent modules to produce an executable,
    regardless of the license terms of these independent modules, and to
    copy and distribute the resulting executable under terms of your choice,
    provided that you also meet, for each linked independent module, the
    terms and conditions of the license of that module. An independent
    module is a module which is not derived from or based on this library.
    If you modify this library, you must extend this exception to your
    version of the library.

    libzmq is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testutil.hpp"
#include "testutil_unity.hpp"

void setUp ()
{
    setup_test_context ();
}

void tearDown ()
{
    teardown_test_context ();
}

void test ()
{
    //  Create a publisher
    void *pub = test_context_socket (ZMQ_PUB);
    TEST_ASSERT_SUCCESS_ERRNO (zmq_bind (pub, "inproc://soname"));

    //  Create two subscribers
    void *sub1 = test_context_socket (ZMQ_SUB);
    TEST_ASSERT_SUCCESS_ERRNO (zmq_connect (sub1, "inproc://soname"));

    void *sub2 = test_context_socket (ZMQ_SUB);
    TEST_ASSERT_SUCCESS_ERRNO (zmq_connect (sub2, "inproc://soname"));

    //  Subscribe pub1 to one prefix
    //  and pub2 to another prefix.
    const char PREFIX1[] = "prefix1";
    const char PREFIX2[] = "p2";

    TEST_ASSERT_SUCCESS_ERRNO (
      zmq_setsockopt (sub1, ZMQ_SUBSCRIBE, PREFIX1, strlen (PREFIX1)));
    TEST_ASSERT_SUCCESS_ERRNO (
      zmq_setsockopt (sub2, ZMQ_SUBSCRIBE, PREFIX2, strlen (PREFIX2)));

    //  Send a message with the first prefix
    send_string_expect_success (pub, PREFIX1, 0);
    msleep (SETTLE_TIME);

    //  sub1 should receive it, but not sub2
    recv_string_expect_success (sub1, PREFIX1, ZMQ_DONTWAIT);

    TEST_ASSERT_FAILURE_ERRNO (EAGAIN, zmq_recv (sub2, NULL, 0, ZMQ_DONTWAIT));

    //  Send a message with the second prefix
    send_string_expect_success (pub, PREFIX2, 0);
    msleep (SETTLE_TIME);

    //  sub2 should receive it, but not sub1
    recv_string_expect_success (sub2, PREFIX2, ZMQ_DONTWAIT);

    TEST_ASSERT_FAILURE_ERRNO (EAGAIN, zmq_recv (sub1, NULL, 0, ZMQ_DONTWAIT));

    //  Now invert the matching
    int invert = 1;
    TEST_ASSERT_SUCCESS_ERRNO (
      zmq_setsockopt (pub, ZMQ_INVERT_MATCHING, &invert, sizeof (invert)));

    //  ... on both sides, otherwise the SUB socket will filter the messages out
    TEST_ASSERT_SUCCESS_ERRNO (
      zmq_setsockopt (sub1, ZMQ_INVERT_MATCHING, &invert, sizeof (invert)));
    TEST_ASSERT_SUCCESS_ERRNO (
      zmq_setsockopt (sub2, ZMQ_INVERT_MATCHING, &invert, sizeof (invert)));

    //  Send a message with the first prefix
    send_string_expect_success (pub, PREFIX1, 0);
    msleep (SETTLE_TIME);

    //  sub2 should receive it, but not sub1
    recv_string_expect_success (sub2, PREFIX1, ZMQ_DONTWAIT);

    TEST_ASSERT_FAILURE_ERRNO (EAGAIN, zmq_recv (sub1, NULL, 0, ZMQ_DONTWAIT));

    //  Send a message with the second prefix
    send_string_expect_success (pub, PREFIX2, 0);
    msleep (SETTLE_TIME);

    //  sub1 should receive it, but not sub2
    recv_string_expect_success (sub1, PREFIX2, ZMQ_DONTWAIT);

    TEST_ASSERT_FAILURE_ERRNO (EAGAIN, zmq_recv (sub2, NULL, 0, ZMQ_DONTWAIT));

    //  Clean up.
    test_context_socket_close (pub);
    test_context_socket_close (sub1);
    test_context_socket_close (sub2);
}

int main ()
{
    setup_test_environment ();

    UNITY_BEGIN ();
    RUN_TEST (test);
    return UNITY_END ();
}
