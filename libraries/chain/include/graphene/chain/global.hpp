/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#define SECONDS_OF_ONE_DAYS 86400 //1 day = 86400 seconds

#pragma once
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {
   class database;
   using namespace graphene::db;
   using namespace graphene::chain;
   using namespace std;
   
string word_contain_sensitive_word(string& word, graphene::chain::database& d);
string string_contain_sensitive_word(string& s, graphene::chain::database& d);
} }



//error no
////////////////////
// 101XXXXX: 预测主题
// 10101XXX：创建预测主题
// 10102XXX：预测主题投票
////////////////////

///////////////////
// 102XXXXX: 通证
// 10201XXX: 创建通证
// 10202XXX: 认购通证
///////////////////

//////////////////
// 103XXXXX: 转账 
// 10301XXX: 普通转账， 详细定义见transfer_evaluator.cpp
// 10302XXX: 批量转账
// 10303XXX: 延迟转账， 详细定义见delay_transfer_evaluator.cpp
//////////////////
