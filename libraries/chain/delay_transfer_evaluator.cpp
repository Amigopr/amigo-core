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
#include <graphene/chain/delay_transfer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

namespace graphene { namespace chain {
/**************************************************************************************
延迟转账，最少一次1笔延迟转账， 最多一次笔延迟转账，支持备注，备注最长36个字符
延迟转账的时间(解冻时间)不能早于当前时间的1小时以内，不超过当前时间的5年之后
按延迟转账的转账笔数来算手续费，每笔延迟转账的计费方式为: 基础费用(0.1个AGC) + 时间费用(每小时0.01AGC)

errno号码段划分见global.hpp
延迟转账errno
10303001: 转账列表的转账接收账号个数不能为0个
10303002: 转账列表里的转账接收账号个数超出限制
10303003: 延迟转账笔数不能为0个
10303004: 延迟转账笔数超出限制
10303005: 延迟转账的时间不能早于当前时间的1小时以内
10303006: 延迟转账的时间已超过当前时间的5年之后
10303007: 只能指定一种资产进行延迟转账
10303008: 备注过长
10303009: 转账接收账人不能是转账发送人
10303010：转账操作时间大于当前区块时间
10303011：转账操作时间不能早于当前区块时间10分钟
****************************************************************************************/
void_result delay_transfer_evaluator::do_evaluate( const delay_transfer_operation& op )
{ try {
   const database& d = db();
   const account_object& from_account = op.from(d);

   try {
      FC_ASSERT(op.to_list.begin() != op.to_list.end(), "errno=10303001, op.to_list.begin() is null. at least 1 receiver for a delay transfer operation. But now the transfer list is empty");
      FC_ASSERT(op.to_list.begin()->second.begin() != op.to_list.begin()->second.end(), 
          "errno=10303003, op.to_list.begin()->second.begin() is null, at least 1 transfer for a delay transfer operation. But now the transfer list is empty");
      FC_ASSERT(op.to_list.begin()->second.size() >= 1, "errno=10303003, op.to_list.begin()->second.begin() is null");

      FC_ASSERT( op.to_list.size() >= 1 && op.to_list.begin() != op.to_list.end(), "errno=10303001, at least 1 receiver for a delay transfer operation. But now the transfer list is empty" );
      FC_ASSERT( op.to_list.size() <= 1, "errno=10303002, the number of transfer exceeds limit(max: 1 receiver for a delay transfer operation)" );

      time_point_sec current = d.head_block_time();

      FC_ASSERT( op.operation_time <= current, "errno 10101013, operation_time ${o} can not be later than current head block time: ${current}",
                      ("o",op.operation_time)("current", current) );
      FC_ASSERT( op.operation_time >= current - fc::minutes(10), "errno=10101014, Creating time ${o} is too early. Operation_time can not be 10 minutes earlier than current head block time ${current} ",
                      ("o",op.operation_time)("current", current) );


      share_type          to_amount_total = 0;
      asset_id_type       asset_id        = op.to_list.begin()->second.begin()->transfer_asset.asset_id;
      const asset_object& asset_type      = asset_id(d);

      //处理每个转账接收人
      for(auto iter=op.to_list.begin(); iter!=op.to_list.end(); ++iter)
      {
         account_id_type                                to = iter->first;//转账接收人
         const account_object&                  to_account = to(d);
         const std::vector<delay_transfer_info>& info_list = iter->second;

         FC_ASSERT( from_account.id != to_account.id, "errno=10303009, the sender and the receiver can not be the same." );

         FC_ASSERT( info_list.size() >= 1 && info_list.begin() != info_list.end(), "errno=10303003, at least 1 transfer for a delay transfer operation. But now the transfer list is empty" );
         FC_ASSERT( info_list.size() <= 1, "errno=10303004, the number of transfer exceeds limit(max: 1 transfers for a delay transfer operation)" );

         //处理每个转账接收人的每一笔转账
         for(auto it=info_list.begin(); it!=info_list.end(); ++it)
         {
            // 转账时间
            // FC_ASSERT( it->transfer_time > op.operation_time + 3600, "errno=10303005, the transfer_time(${t}) is earlier than 1 hour from current time(${c})", ("t", it->transfer_time)("c", d.head_block_time() + 3600));
            FC_ASSERT( it->transfer_time <= d.head_block_time() + 157680000, "errno=10303006, the transfer_time(${t}) is later than 5 years after current time(${c})", 
                    ("t", it->transfer_time)("c", d.head_block_time() + 157680000)); //157680000=60*60*24*365*5

            //FC_ASSERT( it->memo == NULL, "currently memo is not supported" );
            if(it != info_list.begin())
            {
               FC_ASSERT( asset_id == it->transfer_asset.asset_id, "errno=10303007, different asset id ia not permitted( ${a} and ${b})", ("a", asset_id)("b", it->transfer_asset.asset_id));        
            }

            // 当memo为""时，fc::raw::pack_size(it->memo)为1，it->memo->message.size()为一个大数值
            // 当memo为一个108字节字符串时(36个汉字)，fc::raw::pack_size(it->memo)为一个小数值，一般为205。it->memo->message.size()为一个小数值，一般为128
            //ilog("delay_transfer_evaluator::do_evaluate() fc::raw::pack_size(it->memo)=${a}, it->memo->message.size()=${b}", ("a",fc::raw::pack_size(it->memo))("b",it->memo->message.size()));
            FC_ASSERT( fc::raw::pack_size(it->memo)==1 || (it->memo && it->memo->message.size() <= 150), "errno=10303008, memo is too long. memo length=${len}. The max length is 36 characters. to=${to}", 
               ("len", it->memo->message.size())("to", to));

            GRAPHENE_ASSERT(
               is_authorized_asset( d, from_account, asset_type ),
               transfer_from_account_not_whitelisted,
               "'from' account ${from} is not whitelisted for asset ${asset}",
               ("from",op.from)
               ("asset",asset_type.symbol)
               );
            GRAPHENE_ASSERT(
               is_authorized_asset( d, to_account, asset_type ),
               transfer_to_account_not_whitelisted,
               "'to' account ${to} is not whitelisted for asset ${asset}",
               ("to",to)
               ("asset",asset_type.symbol)
               );

            if( asset_type.is_transfer_restricted() )
            {
                GRAPHENE_ASSERT(
                   from_account.id == asset_type.issuer || to_account.id == asset_type.issuer,
                   transfer_restricted_transfer_asset,
                   "Asset {asset} has transfer_restricted flag enabled",
                   ("asset", asset_type.symbol)
                 );
            }
            to_amount_total += it->transfer_asset.amount;

         }//处理每个转账接收人的每一笔转账end
      }

      asset from_balance = d.get_balance( from_account, asset_type );
      asset to_balance(to_amount_total, asset_id);
      bool insufficient_balance = false;

      if( asset_id == op.fee.asset_id )
      {
         insufficient_balance = from_balance.amount >= to_amount_total + op.fee.amount;

         FC_ASSERT( insufficient_balance,
              "errno=10301006, Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}'. Diff is: ${diff}", 
              ("balance",d.to_pretty_string(from_balance))
              ("total_transfer",d.to_pretty_string(to_balance))
              ("a",from_account.name)
              ("diff",d.to_pretty_string(to_balance + op.fee - from_balance) ));
      }
      else
      {
         insufficient_balance = from_balance.amount >= to_amount_total;

         FC_ASSERT( insufficient_balance,
              "errno=10301006, Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}'. Diff is: ${diff}", 
              ("balance",d.to_pretty_string(from_balance))
              ("total_transfer",d.to_pretty_string(to_balance))
              ("a",from_account.name)
              ("diff",d.to_pretty_string(to_balance - from_balance) ));

         asset from_balance_for_fee_asset_type = d.get_balance( op.from, op.fee.asset_id );
         insufficient_balance = from_balance_for_fee_asset_type >= op.fee;
         FC_ASSERT( insufficient_balance,
              "errno=10301007, Insufficient Balance: ${balance}, unable to pay fee '${fee}' from account '${a}'. Diff is: ${diff}", 
              ("balance",d.to_pretty_string(from_balance_for_fee_asset_type))
              ("fee",d.to_pretty_string(op.fee))
              ("a",from_account.name)
              ("diff",d.to_pretty_string(op.fee - from_balance_for_fee_asset_type) ));
      }

      return void_result();

   } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer from ${f}. op=${op}", ("f",op.from(d).name)("op",op ));

}  FC_CAPTURE_AND_RETHROW( (op) ) }


void_result delay_transfer_evaluator::do_apply( const delay_transfer_operation& op )
{ try {
    auto start = fc::time_point::now();
    const database& d            = db();
    //FC_ASSERT(op.to_list.begin(), "op.to_list.begin() is null");
    //FC_ASSERT(op.to_list.begin()->second.begin(), "op.to_list.begin()->second.begin() is null");

    asset_id_type   asset_id     = op.to_list.begin()->second.begin()->transfer_asset.asset_id;
    asset           from_balance = d.get_balance( op.from, asset_id );
    ilog("delay transfer, from=${from}, from_balance=${balance}", ("from", op.from)("balance",d.to_pretty_string(from_balance)));
    asset to_amount_total(0, asset_id);

    //处理每个转账接收人
    for(auto iter=op.to_list.begin(); iter!=op.to_list.end(); ++iter)
    {
        account_id_type                   to        = iter->first;//转账接收人
        const std::vector<delay_transfer_info>& info_list = iter->second;

        //一个延迟转账操作的每个转账接收人对应一个延迟转账object
        db().create<delay_transfer_object>( [&]( delay_transfer_object& obj ){
            obj.from           = op.from;
            obj.to             = to;
            //obj.operation_time = d.head_block_time();
            obj.operation_time = op.operation_time;

            //处理每个转账接收人的每一笔转账
            for(auto it=info_list.begin(); it != info_list.end(); it++)
            {
              ilog("delay transfer, to=${to}, transfer_asset=${transfer_asset}, transfer_time=${time}", 
                    ("to", iter->first)("transfer_asset", it->transfer_asset)("time", it->transfer_time));
              delay_transfer_record record(*it);
              obj.delay_transfer_detail.push_back(record);
              to_amount_total += it->transfer_asset;
            }
        });

        //对1个转账接收人，对他的每一笔转账，处理相应的待解冻的每种资产的明细统计
        //处理delay_transfer_unexecuted_object
        delay_transfer_unexecuted_object* unexecuted_object;
        const auto& unexecuted_object_index = db().get_index_type<delay_transfer_unexecuted_index>().indices().get<by_to>();
        auto itr = unexecuted_object_index.find(to);
        if (itr == unexecuted_object_index.end())//没找到
        {
            ilog("delay_transfer_evaluator::do_apply create delay_transfer_unexecuted_object for to=${to}", ("to", to));
            //create delay_transfer_unexecuted_object
            unexecuted_object = const_cast<delay_transfer_unexecuted_object*>(&(db().create<delay_transfer_unexecuted_object>( [&]( delay_transfer_unexecuted_object& obj ) {
                obj.to = to;
          })));
        }
        else
        {
            unexecuted_object = const_cast<delay_transfer_unexecuted_object*>(&*itr);
        }

        for(auto it=info_list.begin(); it != info_list.end(); it++)
        {
            //处理对每个账号的每种没执行(待解冻)的资产的明细统计
            db().modify(*unexecuted_object, [&](delay_transfer_unexecuted_object& o) 
            {
                auto temp_it = unexecuted_object->unexecuted_asset.find(asset_id);
                if(temp_it == unexecuted_object->unexecuted_asset.end()) //没找到该类资产
                {
                    o.unexecuted_asset.insert(pair<asset_id_type, share_type>(asset_id, it->transfer_asset.amount));
                }
                else//没找到该类资产
                {
                    o.unexecuted_asset[asset_id] = temp_it->second + it->transfer_asset.amount;
                }
            });
        }
    }// for

    db().adjust_balance( op.from, -to_amount_total );

    from_balance = d.get_balance( op.from, asset_id );
    ilog("delay transfer end, from_balance=${balance}, to_account_number=${num}, to_amount_total=${amount}, fee=${fee}", 
         ("balance", d.to_pretty_string(from_balance))("num", op.to_list.size())("amount", to_amount_total)("fee",d.to_pretty_string(op.fee)));

    auto end = fc::time_point::now();
    ilog( "delay_transfer_evaluator::do_apply, elapsed time: ${t} msec", ("t",double((end-start).count())/1000.0 ) );
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }
} } // graphene::chain
