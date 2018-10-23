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

#include <graphene/chain/protocol/delay_transfer.hpp>

namespace graphene { namespace chain {


void delay_transfer_operation::validate()const
{
	FC_ASSERT( fee.amount >= 0 );
}

share_type delay_transfer_operation::calculate_fee( const fee_parameters_type& schedule )const
{
	share_type core_fee_required = 0;

    //处理每个转账接收人
    for(auto iter=to_list.begin(); iter!=to_list.end(); ++iter)
    {
    	//处理每个转账接收人的每一笔转账
    	const std::vector<delay_transfer_info>& info_list = iter->second;
        for(auto it=info_list.begin(); it!=info_list.end(); ++it)
        {
            // 转账时间
            //ilog("operation_time=${o}, transfer_time=${t}", ("o", operation_time)("t", it->transfer_time));
            FC_ASSERT( it->transfer_time > operation_time, "errno=10303005, operation_time=${o}, the transfer_time(${t}) is earlier than operation_time(${c}). ", 
            		("o", operation_time)("t", it->transfer_time)("c", operation_time));
            FC_ASSERT( it->transfer_time <= operation_time + 157680000, "errno=10303006, operation_time=${o}, the transfer_time(${t}) is later than 5 years after operation_time(${c})", 
                    ("o", operation_time)("t", it->transfer_time)("c", operation_time + 157680000)); //157680000=86400*365*5
			core_fee_required += schedule.basic_fee + ((it->transfer_time - operation_time).to_seconds()) / 3600 * schedule.price_per_hour;
		}
	}
	
    return core_fee_required;
}

} } // graphene::chain
