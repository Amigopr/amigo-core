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
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/token_object.hpp>
#include <graphene/chain/protocol/token_profile.hpp>

namespace graphene { namespace chain {

bool check_validity_for_url(const string& article_url);
token_change_profie get_token_profile(const variant_object& value, bool get_reserved_asset_name_and_symbol=true);

class token_publish_evaluator : public evaluator<token_publish_evaluator>
{
public:
   typedef token_publish_operation operation_type;

   void_result do_evaluate( const token_publish_operation& o );
   object_id_type do_apply( const token_publish_operation& o );

   virtual void pay_fee() override;
   share_type	deferred_fee_ =0;
   time_point_sec create_time_;
   time_point_sec phase1_begin_time_;
   time_point_sec phase1_end_time_;
   time_point_sec phase2_begin_time_;
   time_point_sec phase2_end_time_;
   time_point_sec settle_time_;
   //time_point_sec return_asset_end_time_;
   
   const account_object* issuer_     = nullptr;
   string upper_case_asset_name_;
};

class token_buy_evaluator : public evaluator<token_buy_evaluator>
{
public:
   typedef token_buy_operation operation_type;

   void_result do_evaluate( const token_buy_operation& o );
   object_id_type do_apply( const token_buy_operation& o );


   /** override the default behavior defined by generic_evalautor which is to
    * post the fee to fee_paying_account_stats.pending_fees
   */
   virtual void pay_fee() override;
   share_type  deferred_fee_ =0;

   //const account_object* buyer_     = nullptr;
   //const asset_object*  buy_asset_ = nullptr;
};

class token_event_evaluator : public evaluator<token_event_evaluator>
{
public:
   typedef token_event_operation operation_type;

   void_result do_evaluate( const token_event_operation& o );
   object_id_type do_apply( const token_event_operation& o );
};

class token_update_evaluator : public evaluator<token_update_evaluator>
{
public:
   typedef token_update_operation operation_type;

   void_result do_evaluate( const token_update_operation& o );
   void_result do_apply( const token_update_operation& o );
};

} } // graphene::chain