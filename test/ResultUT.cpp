/*
* Copyright (c) 2018 Pawel Drzycimski
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include <gtest/gtest.h>
#include <simpleopenssl/simpleopenssl.h>
#include "utils.h"

namespace so { namespace ut { namespace expected {


TEST(ResultUT, okUsageWithBytes)
{
  // WHEN
  const auto expected = ::so::internal::ok(::so::Bytes{0x00, 0x01});
 
  // THEN
  EXPECT_TRUE(expected);
  EXPECT_TRUE(expected.hasValue());
  EXPECT_FALSE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(0), expected.errorCode());
  EXPECT_EQ("ok", expected.msg());
  EXPECT_EQ((::so::Bytes{0x00, 0x01}), *expected);
  EXPECT_EQ((::so::Bytes{0x00, 0x01}), expected.value());
}

TEST(ResultUT, errUsageWithBytes)
{
  // WHEN
  const auto expected = ::so::internal::err<::so::Bytes>(5);
 
  // THEN
  EXPECT_FALSE(expected);
  EXPECT_FALSE(expected.hasValue());
  EXPECT_TRUE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(5), expected.errorCode());
  EXPECT_EQ((::so::Bytes{}), *expected);
  EXPECT_EQ((::so::Bytes{}), expected.value());
}

TEST(ResultUT, okUsageWithUptrs)
{
  // WHEN
  const auto expected = ::so::internal::ok(::so::make_unique<BIGNUM>(nullptr));

  // THEN
  EXPECT_TRUE(expected);
  EXPECT_TRUE(expected.hasValue());
  EXPECT_FALSE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(0), expected.errorCode());
  EXPECT_EQ("ok", expected.msg());
}

TEST(ResultUT, errUsageWithUptrs)
{
  // WHEN 
  const auto expected = ::so::internal::err<::so::BIGNUM_uptr>(5);
 
  // THEN
  EXPECT_FALSE(expected);
  EXPECT_FALSE(expected.hasValue());
  EXPECT_TRUE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(5), expected.errorCode());
}

TEST(ResultUT, okUsageWithVoid)
{
  // WHEN
  const auto expected = ::so::internal::ok(); 

  // THEN  
  EXPECT_TRUE(expected);
  EXPECT_FALSE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(0), expected.errorCode());
  EXPECT_EQ("ok", expected.msg());
}

TEST(ResultUT, errUsageWithVoid)
{
  // WHEN  
  const auto expected = ::so::internal::err<void>(5);
 
  // THEN
  EXPECT_FALSE(expected);
  EXPECT_TRUE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(5), expected.errorCode());
}

TEST(ResultUT, okUsageWithUnsignedLong)
{
  // WHEN
  const auto expected = ::so::internal::ok(10ul); 

  // THEN  
  EXPECT_TRUE(expected);
  EXPECT_TRUE(expected.hasValue());
  EXPECT_EQ(10ul, *expected);
  EXPECT_EQ(10ul, expected.value());
  EXPECT_FALSE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(0), expected.errorCode());
  EXPECT_EQ("ok", expected.msg());
}

TEST(ResultUT, errUsageWithUnsignedLong)
{
  // WHEN  
  const auto expected = ::so::internal::err<unsigned long>(5);
 
  // THEN
  EXPECT_FALSE(expected);
  EXPECT_FALSE(expected.hasValue());
  EXPECT_TRUE(expected.hasError());
  EXPECT_EQ(static_cast<unsigned long>(5), expected.errorCode());
}

}}} // namespace so { namespace ut { namespace bignum {
