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
 * furnished to do so, delay_transfer to the following conditions:
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


#include <graphene/chain/database.hpp>
#include <graphene/chain/delay_transfer_object.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <stdio.h>
namespace graphene { namespace chain {

int database::delay_transfer_transition()
{
	auto& delay_transfer_indexs = get_index_type<delay_transfer_index>().indices().get<by_id>();
	auto iter = delay_transfer_indexs.begin();
    auto delay_transfer_end = delay_transfer_indexs.end();

    const uint number_of_handle_delay_transfer_per_block = 30; //每个出块周期最多处理的延迟转账object(不是指延迟转账记录)的个数
    static uint64_t handle_begin_index = 1; //从整个集合的第一个延迟转账开始处理，每个出块周期会记录下个周期一开始要处理的延迟转账的记录位置
    
    uint64_t handle_count = 0; //本出块周期已处理的延迟转账操作的个数
    uint64_t index = 1;
    bool skip = true;

	while(iter != delay_transfer_end && handle_count < number_of_handle_delay_transfer_per_block ) 
	{
		if(skip && index < handle_begin_index) //先跳到本出块周期一开始要处理的延迟转账的记录位置
      	{
      		++index;
      		++iter;
      		continue;
      	}
      	else if (skip)
      	{
      		skip = false;
      	}

		const delay_transfer_object& delay_transfer = *iter;
      	bool bFinishForOneObject = true;

      	if( !delay_transfer.finished ) //处理1个延迟转账object
      	{
      		int size = delay_transfer.delay_transfer_detail.size();
      		for(int i = 0; i < size; ++i)//处理1个延迟转账object的每一笔延迟转账
	      	{
	      		// 需要发放给转账接收人
	      		if( !delay_transfer.delay_transfer_detail[i].executed && delay_transfer.delay_transfer_detail[i].info.transfer_time <= head_block_time() )
	      		{
	      			adjust_balance(delay_transfer.to, delay_transfer.delay_transfer_detail[i].info.transfer_asset);

					modify(delay_transfer, [&](delay_transfer_object& obj) {
								obj.delay_transfer_detail[i].executed = true;
								obj.delay_transfer_detail[i].execute_time = head_block_time();
					});

					//处理对1个接收账号的1种没执行(待解冻)的资产的统计
					asset_id_type asset_id = delay_transfer.delay_transfer_detail[i].info.transfer_asset.asset_id;
			        auto& unexecuted_index = get_index_type<delay_transfer_unexecuted_index>().indices().get<by_to>();
			        auto itr = unexecuted_index.find(delay_transfer.to);

			        FC_ASSERT( itr != unexecuted_index.end(), "can not find unexecuted_object, to=${to}", ("to", delay_transfer.to));//没找到转账接收人
			        FC_ASSERT( itr->unexecuted_asset.count(asset_id) > 0 , "can not find the specified asset in the unexecuted_object, to=${to}, asset_id=${a}", 
			        		("to", delay_transfer.to)("a", asset_id));//没找到对应的资产
			        //没执行(待解冻)的资产不能为负数
			        delay_transfer_unexecuted_object *temp = const_cast<delay_transfer_unexecuted_object*>(&*itr);
			        share_type amount = temp->unexecuted_asset[asset_id];
			        FC_ASSERT( amount >= delay_transfer.delay_transfer_detail[i].info.transfer_asset.amount, 
			        		"itr->unexecuted_asset.[asset_id] is wrong. to=${to}, asset_id=${a}, itr->unexecuted_asset.[asset_id]=${i}, will_executed_asset=${w}", 
			        		("to", delay_transfer.to)("a", asset_id)("i", amount)("w", delay_transfer.delay_transfer_detail[i].info.transfer_asset.amount));

			        modify(*itr, [&](delay_transfer_unexecuted_object& o) 
			        {
						o.unexecuted_asset[asset_id] = o.unexecuted_asset[asset_id] - delay_transfer.delay_transfer_detail[i].info.transfer_asset.amount;
			        });

					ilog("delay_transfer_transition id=${delay_transfer}, from=${from}, to=${to}, time=${time} now=${now}, transfer_asset=${a}", 
							("delay_transfer", iter->id)("from", delay_transfer.from)("to", delay_transfer.to)("time", delay_transfer.delay_transfer_detail[i].info.transfer_time)
							("now", head_block_time())("a", delay_transfer.delay_transfer_detail[i].info.transfer_asset));
	      		}
	      		else if (!delay_transfer.delay_transfer_detail[i].executed && delay_transfer.delay_transfer_detail[i].info.transfer_time > head_block_time())
				{
					bFinishForOneObject = false;
				}
			}

			// 该延迟转账object结束
			if( bFinishForOneObject )
			{
				modify(delay_transfer, [&](delay_transfer_object& obj) {
								obj.finished = true;
				});

				ilog("delay_transfer_transition finished. id=${delay_transfer}, from=${from}, to=${to}, now=${now}", 
						("delay_transfer", delay_transfer.id)("from", delay_transfer.from)("to", delay_transfer.to)("now", head_block_time()));
			}
      	}

		++iter;
      	++handle_begin_index;
      	++handle_count;
	    if (iter == delay_transfer_end)
	    {
	    	handle_begin_index = 1;//下个出块周期从第一个延迟转账object开始处理
	    }

	} //while
	return 0;
}

} } // graphene::chain
