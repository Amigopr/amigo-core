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
 * furnished to do so, token to the following conditions:
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
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain { 
   struct delay_transfer_info
   {
      delay_transfer_info(){}

      delay_transfer_info( asset a, time_point_sec t, optional<memo_data> m = NULL )
      :transfer_asset(a), transfer_time(t), memo(m){}

      asset               transfer_asset; // 转账资产
      time_point_sec      transfer_time;  // 转账时间
      optional<memo_data> memo; //备注
   };

   struct delay_transfer_operation : public base_operation
   {
      struct fee_parameters_type {
         //uint64_t fee       = 50000000; // 0.5 AGC per a transfer
         uint64_t basic_fee       = 0.1 * GRAPHENE_BLOCKCHAIN_PRECISION;       //基础费用0.1个代币
         uint64_t price_per_hour  = 0.01 * GRAPHENE_BLOCKCHAIN_PRECISION;    // 0.01AGC/hour
         //uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
      };

      asset             fee;
      account_id_type   from; //转账发送人
      time_point_sec    operation_time;//转账操作时间
      
      /// A list of to_info
      map<account_id_type, vector<delay_transfer_info>>   to_list; //account_id_type=转账接收人
            
      extensions_type   extensions;

      account_id_type fee_payer()const { return from; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
   };
} } // graphene::chain


FC_REFLECT(graphene::chain::delay_transfer_info, (transfer_asset)(transfer_time)(memo))

FC_REFLECT( graphene::chain::delay_transfer_operation::fee_parameters_type, (basic_fee)(price_per_hour) )
FC_REFLECT( graphene::chain::delay_transfer_operation, (fee)(from)(operation_time)(to_list)(extensions) )