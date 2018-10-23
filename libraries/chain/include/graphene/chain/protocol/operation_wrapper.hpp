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
//#include <graphene/chain/protocol/types.hpp>


namespace graphene { namespace chain { 

 
   /**
    * 内部使用的操作协议operation_wrapper_operation，用于临时的紧急修改 
    */
   struct operation_wrapper_operation : public base_operation
   {
      struct fee_parameters_type
   	  {
        uint64_t fee = 1000000; // 0.01 AGC
      };

   	  asset           		  fee;
      account_id_type       oper;//操作者
   	  string                operation_name;
      map<string, string>   parameters;

      extensions_type       extensions;


      account_id_type fee_payer()const { return oper; }
      void            validate()const;
      share_type      calculate_fee( const fee_parameters_type& k )const;
   };


} } // graphene::chain


FC_REFLECT( graphene::chain::operation_wrapper_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::operation_wrapper_operation, (fee)(operation_name)(parameters)(extensions) )