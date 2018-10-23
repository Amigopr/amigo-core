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

#pragma once
#include <graphene/chain/protocol/delay_transfer.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {
   class account_object;
   class database;
   using namespace graphene::db;

   struct delay_transfer_record
   {
      delay_transfer_record(){}

      delay_transfer_record( graphene::chain::delay_transfer_info i )
      :info(i){}
      graphene::chain::delay_transfer_info info;
      bool              executed = false; //是否已发放给转账接收人
      time_point_sec    execute_time;  // 发放时间

   };
   

   /**
    * 延迟转账，每个转账接收人对应一个object
    */
   class delay_transfer_object: public graphene::db::abstract_object<delay_transfer_object>
   {
      public:
        static const uint8_t space_id = protocol_ids;
        static const uint8_t type_id  = delay_transfer_object_type;

        account_id_type                     from;//转账发送人
        account_id_type                     to; //转账接收人
        time_point_sec                      operation_time;//转账操作时间
        std::vector<delay_transfer_record>  delay_transfer_detail; // 转账记录
        bool                                finished = false;  //是否全部发放完毕

        optional<map<string, string>>       exts; // extend_options
   };


   struct by_from;
   struct by_to;
   struct by_operation_time;
   struct by_delay_transfer_finished;

   typedef multi_index_container<
      delay_transfer_object,
      indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_non_unique< tag<by_from>, member<delay_transfer_object, account_id_type, &delay_transfer_object::from> >,
          ordered_non_unique< tag<by_to>, member<delay_transfer_object, account_id_type, &delay_transfer_object::to> >,
          ordered_non_unique< tag<by_operation_time>, member<delay_transfer_object, time_point_sec, &delay_transfer_object::operation_time> >,
          ordered_non_unique< tag<by_delay_transfer_finished>, member<delay_transfer_object, bool, &delay_transfer_object::finished> >
		  >
   > delay_transfer_object_multi_index_type;
   typedef generic_index<delay_transfer_object, delay_transfer_object_multi_index_type> delay_transfer_index;


   /**
   * 延迟转账，每个转账接收人对应一个object
   */
   class delay_transfer_object_for_query: public graphene::db::abstract_object<delay_transfer_object>
   {
      public:
        static const uint8_t space_id = protocol_ids;
        static const uint8_t type_id  = delay_transfer_object_type;

        account_id_type                     from;//转账发送人
        string                              from_name;//转账发送人账号
        account_id_type                     to; //转账接收人
        string                              to_name;//转账接收人账号
        string                              transfer_asset_symbol;//转账资产缩写
        time_point_sec                      operation_time;//转账操作时间
        std::vector<delay_transfer_record>  delay_transfer_detail; // 转账记录
        bool                                finished = false;  //是否全部发放完毕

        optional<map<string, string>>       exts; // extend_options
   };

    /**
    * 延迟转账中，对每个账号的每种没执行(解冻)的资产的明细统计，每个转账接收人对应一个object
    */
   class delay_transfer_unexecuted_object : public graphene::db::abstract_object<delay_transfer_unexecuted_object>
   {
      public:
        static const uint8_t space_id = implementation_ids;
        static const uint8_t type_id  = impl_delay_transfer_unexecuted_object_type;

        account_id_type                     to; //转账接收人
        map<asset_id_type, share_type>      unexecuted_asset; //没执行(待解冻)的资产 map<资产id, 资产数量>
   };

   struct by_to;

   typedef multi_index_container<
      delay_transfer_unexecuted_object,
      indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_non_unique< tag<by_to>, member<delay_transfer_unexecuted_object, account_id_type, &delay_transfer_unexecuted_object::to> >
      >
   > delay_transfer_unexecuted_object_multi_index_type;
   typedef generic_index<delay_transfer_unexecuted_object, delay_transfer_unexecuted_object_multi_index_type> delay_transfer_unexecuted_index;

} } // graphene::chain


FC_REFLECT(graphene::chain::delay_transfer_record, (info)(executed)(execute_time))

FC_REFLECT_DERIVED( graphene::chain::delay_transfer_object, (graphene::db::object),
                    (from)(to)(operation_time)(delay_transfer_detail)(finished)(exts)
                  )

FC_REFLECT_DERIVED( graphene::chain::delay_transfer_object_for_query, (graphene::db::object),
                    (from)(from_name)(to)(to_name)(transfer_asset_symbol)(operation_time)(delay_transfer_detail)(finished)(exts)
                  )

FC_REFLECT_DERIVED( graphene::chain::delay_transfer_unexecuted_object, (graphene::db::object),
                    (to)(unexecuted_asset)
                  )