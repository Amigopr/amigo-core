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

#include <graphene/app/database_api.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/chain/token_evaluator.hpp>
#include <graphene/chain/global.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>
#include <algorithm>  

#define GET_REQUIRED_FEES_MAX_RECURSION 4
#define MAX_SUBJECT_NUM_FOR_QUERY_RESULTS 30
#define MAX_TOKEN_NUM_FOR_QUERY_RESULTS 30
#define MAX_BUY_TOKEN_RECORD_NUM_FOR_QUERY_RESULTS 100
#define MAX_GET_BUY_LIST_RECORD_NUM 100000
#define MAX_DELAY_TRANSFER_NUM_FOR_QUERY_RESULTS 50

typedef std::map< std::pair<graphene::chain::asset_id_type, graphene::chain::asset_id_type>, std::vector<fc::variant> > market_queue_type;

namespace graphene { namespace app {

class database_api_impl;


class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      database_api_impl( graphene::chain::database& db );
      ~database_api_impl();

	  enum direction_type
	  {  
		  negative_sequence = 0,
		  positive_sequence = 1
	  };

	  enum query_token_type{
		  market_tokens_query,
		  my_tokens_query,
	  };



      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      map<uint32_t, optional<block_header>> get_block_header_batch(const vector<uint32_t> block_nums)const;
      optional<signed_block> get_block(uint32_t block_num)const;
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

      // Globals
      chain_property_object get_chain_properties()const;
      global_property_object get_global_properties()const;
      fc::variant_object get_config()const;
      chain_id_type get_chain_id()const;
      dynamic_global_property_object get_dynamic_global_properties()const;

      // Keys
      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;
     bool is_public_key_registered(string public_key) const;

      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      optional<account_object> get_account_by_name( string name )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      // Balances
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;
      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;
      vector<asset> get_vested_balances( const vector<balance_id_type>& objs )const;
      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

      // Assets
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;
      vector<asset_object>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      module_cfg_object get_module_cfg(const string& module_name)const;

      // Markets / feeds
      vector<limit_order_object>         get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const;
      vector<call_order_object>          get_call_orders(asset_id_type a, uint32_t limit)const;
      vector<force_settlement_object>    get_settle_orders(asset_id_type a, uint32_t limit)const;
      vector<call_order_object>          get_margin_positions( const account_id_type& id )const;
      void subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b);
      void unsubscribe_from_market(asset_id_type a, asset_id_type b);
      market_ticker                      get_ticker( const string& base, const string& quote )const;
      market_volume                      get_24_volume( const string& base, const string& quote )const;
      order_book                         get_order_book( const string& base, const string& quote, unsigned limit = 50 )const;
      vector<market_trade>               get_trade_history( const string& base, const string& quote, fc::time_point_sec start, fc::time_point_sec stop, unsigned limit = 100 )const;

      // Witnesses
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;
      fc::optional<witness_object> get_witness_by_account(account_id_type account)const;
      map<string, witness_id_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_witness_count()const;

      // Committee members
      vector<optional<committee_member_object>> get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const;
      fc::optional<committee_member_object> get_committee_member_by_account(account_id_type account)const;
      map<string, committee_member_id_type> lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const;

      // Votes
      vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      set<address> get_potential_address_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx )const;
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      // Proposed transactions
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;

      // Blinded balances
      vector<blinded_balance_object> get_blinded_balances( const flat_set<commitment_type>& commitments )const;

	  //[lilianwen add 2018-1-30]通证查询接口
	  std::vector<token_brief> get_tokens_brief(const token_query_condition &condition)const;//通证简介查询批量接口
	  //从“我的通证”界面点击查询
	  std::vector<token_brief> my_get_tokens_brief(const token_query_condition &condition)const;//通证简介查询批量接口
	  //从“我的通证”界面点击查询我的创建
	  std::vector<token_brief> my_get_my_create_tokens_brief(const token_query_condition &condition)const;//通证简介查询批量接口
	  //从通用界面点击查询符号或者通证id返回通证简介
	  optional<token_brief> get_token_brief_by_symbol_or_id(const string &token_symbol_or_id, const string &my_account)const;
	  //从“我的通证”界面点击查询符号或者通证id返回通证简介
	  optional<token_brief> my_get_token_brief_by_symbol_or_id(const string &token_symbol_or_id, const string &my_account)const;

	  //点击具体某个token界面的时候调用这个接口
	  optional<token_detail>   get_token_detail(const string &token_symbol_or_id, const string &my_account)const;

	  //认购明细查询，查询account_name_or_id这个账号对token_id的所有认购详情
	  vector<token_buy_object> get_buy_token_detail(object_id_type token_id, const string &account_name_or_id)const;


	  //查询认购名单的总数量（单位人次），用户客户端显示进度条用
	  uint32_t get_buy_record_total(object_id_type token_id, const string &issue_account)const;


    int is_asset_name_valid(const string asset_name);
    int is_asset_symbol_valid(const string asset_symbol);

	  //导出认购名单
	  //start和limit用于分页查询
	  //account_name_or_id用来传入项目创建者的账号或者id，只有项目的创建者才能成功调用这个接口
	  vector<token_buy_object> get_buy_list(uint32_t start, uint32_t limit, object_id_type token_id, const string &issue_account)const;
	  
	  optional<token_brief> get_token_brief_by_symbol_or_id_impl(const string &token_symbol_or_id, const string &my_account,  query_token_type type)const;
	  std::vector<token_brief> get_tokens_brief_impl(const token_query_condition &condition, query_token_type type)const;
	  void TokenFillExtendField(const optional<map<string, string> >  &token_exts, const set<string> &ext_field,map<string, string> &ret_ext_field)const;
	  //[end]

      std::vector<token_object> get_tokens_by_collected_core_asset( uint32_t start, uint32_t limit )const;
      optional<token_object> get_token_by_id( token_id_type token_id )const;
      optional<token_object> get_token_by_asset_name( string asset_name )const;
      size_t get_token_total() const;

	template<typename T>
	std::vector<token_brief> get_tokens_brief_template_by_statistics_negative(const token_query_condition &condition, query_token_type type)const
	{
		std::vector<token_brief> result;
        if(condition.limit == 0 ) return {};
		const auto &idx = _db.get_index_type<token_statistics_index>().indices().get<T>();
		if(idx.size() == 0) return {};
        
		//考虑end_time 往前推的情况
        uint32_t count=0;
		auto itr     = idx.end();
        itr--;
		for(;true;itr--)
		{
            if(count+1 <= condition.start)
            {
                count++;
                if(itr == idx.begin()) break;
                continue;
            }
        
            token_brief one;
        
            one.token_id                   = itr->token_id;
            one.actual_buy_amount          = itr->actual_buy_total;//所有参与的用户已经认购的用户资产数量，这个考虑一下怎么算
            one.actual_core_asset_total    = itr->actual_core_asset_total;//所有参与的用户已经募集的AGC数量
            one.buyer_number               = itr->buyer_number;
        
            token_object* _token = (token_object*)_db.find_object(one.token_id);
            if (_token == NULL)//主题创建了，但是还没有投票
            {
                elog("token not found.token id ${id} is not found.", ("id", one.token_id));
            }
            else
            {
                if ( condition.status != "all" )
                {
                    if ( condition.status == "end" && _token->status < token_object::settle_status ) 
                    {
                    	if(itr == idx.begin()) break;
                    	continue;
                    }
                    if ( condition.status == "buy" && (_token->status >= token_object::settle_status || _token->status <=token_object::create_status) )
                    {
                    	if(itr == idx.begin()) break;
                    	continue;
                    } 
                }
                
                if (_token->control == token_object::token_control::unavailable)
                {
                	if(itr == idx.begin()) break;
                    continue;
                }
                else if (_token->control == token_object::token_control::description_forbidden)
                {
                    one.brief       = "This token is description forbidden.";
                    one.description = "This token is description forbidden.";
                }
                else
                {
                    one.brief       = _token->template_parameter.brief;
                    one.description = _token->template_parameter.description;
                }
            
                one.status             = _token->status;
                //根据id查询名字
                one.issuer             = _db.find(_token->issuer)->name;
                one.guaranty_credit    = _token->guaranty_credit;
                one.control            = _token->control;
                one.logo_url           = _token->template_parameter.logo_url;
                one.asset_name         = _token->template_parameter.asset_name;
                one.asset_symbol       = _token->template_parameter.asset_symbol;
                one.max_supply         = _token->template_parameter.max_supply;//token的最大供应量
                one.plan_buy_total     = _token->template_parameter.plan_buy_total;//要改成plan_buy_total
                one.buy_succeed_min_percent = _token->template_parameter.buy_succeed_min_percent;
                one.need_raising       = _token->template_parameter.need_raising;
        
                one.create_time  = _token->status_expires.create_time;
                one.phase1_begin = _token->status_expires.phase1_begin; // 认购第1阶段开始时间
                one.phase1_end   = _token->status_expires.phase1_end; // 认购第1阶段结束时间
                one.phase2_begin = _token->status_expires.phase2_begin; // 认购第2阶段开始时间
                one.phase2_end   = _token->status_expires.phase2_end;
                one.settle_time  = _token->status_expires.settle_time;//token的结算时间
                one.guaranty_core_asset_amount = _token->template_parameter.guaranty_core_asset_amount;//抵押AGC数量
            }
        
            one.buy_count = 0;
            //查看当前账户是否投过票
            if ( condition.my_account == "" || condition.my_account == "null" )
            {
                elog("account name or id [${account}] is empty.", ("account",condition.my_account) );
                //one.subject_vote_id = optional<object_id_type>();
            }
            else
            {
                const account_object* account = nullptr;
                if (std::isdigit(condition.my_account[0]))
                    account = _db.find(fc::variant(condition.my_account).as<account_id_type>());
                else
                {
                    const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
                    auto itr = idx.find(condition.my_account);//如果这里为字符串null的话会导致程序崩溃
                    if (itr != idx.end())
                        account = &*itr;
                }
                if (account != nullptr)
                {
                    //创建者也算是参与者
                    if(_token->issuer == account->id )
                    {
                        one.buy_count++;
                    }
        
                    const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_id>();
                    auto itr_buy = idx_buy.begin();
        
                    for(;itr_buy != idx_buy.end(); itr_buy++)
                    {
                        if(itr_buy->buyer == account->id && itr_buy->token_id == one.token_id)
                        {
                            one.buy_count++;
                            if (type == my_tokens_query)
                            {
                                one.my_participate.push_back(itr_buy->template_parameter);
                            }
                            else
                                break;
                        }
                    }
                }
            }
        
            //处理扩展字段
            TokenFillExtendField(_token->exts, condition.extra_query_fields, one.extend_field);
            if ( type == my_tokens_query )
            {
                if (one.buy_count)
                {
                    result.push_back(one);
                }
            }
            else
            {
                result.push_back(one);
            }
        
            if(itr == idx.begin()) break;
            if(result.size() == condition.limit) break;
		}
        return result;		    
	}

    template<typename T>
    std::vector<token_brief> get_tokens_brief_template_by_global_positive(const token_query_condition &condition, query_token_type type, const string &ext="")const
    {
    	if (condition.limit == 0) return {};
        if (condition.start_time > condition.end_time)
        {
            elog("start time[${stime}] is larger than end time[${etime}]", ("stime", condition.start_time)("etime", condition.end_time));
            return {};
        }

        const auto &idx = _db.get_index_type<token_index>().indices().get<T>();
        if(idx.size() == 0) return {};

        std::vector<token_brief> result;
        uint32_t count=0;
        auto itr     = idx.begin();
        auto itr_end = idx.end();
        for(;itr != itr_end;itr++)
        {      
            if ( condition.status != "all" )
            {
                if ( condition.status == "end" && itr->status < token_object::settle_status ) continue;
                if ( condition.status == "buy" && (itr->status >= token_object::settle_status || itr->status <=token_object::create_status) ) continue;
            }
            
            //if(typeid(T).name() == "by_create_time")
            if(ext == "by_create_time")
            {
                if(itr->status_expires.create_time < condition.start_time || itr->status_expires.create_time > condition.end_time)
                {
                    continue;
                }
            }
            //else if(typeid(T).name() == "by_end_time")
            else if(ext == "by_end_time")
            {
                if(itr->status_expires.settle_time < condition.start_time || itr->status_expires.settle_time > condition.end_time)
                {
                    continue;
                }                
            }
            
            if(count+1 <= condition.start)
            {
                count++;
                continue;
            }

            token_brief one;

            if (itr->control == token_object::token_control::unavailable)
            {
                continue;
            }
            else if (itr->control == token_object::token_control::description_forbidden)
            {
                one.brief       = "This token is description forbidden.";
                one.description = "This token is description forbidden.";
            }
            else
            {
                one.brief       = itr->template_parameter.brief;
                one.description = itr->template_parameter.description;
            }

            one.status             = itr->status;
            //根据id查询名字
            one.issuer             = _db.find(itr->issuer)->name;
            one.guaranty_credit    = itr->guaranty_credit;
            one.control            = itr->control;
            one.logo_url           = itr->template_parameter.logo_url;
            one.asset_name         = itr->template_parameter.asset_name;
            one.asset_symbol       = itr->template_parameter.asset_symbol;
            one.max_supply         = itr->template_parameter.max_supply;//token的最大供应量
            one.plan_buy_total     = itr->template_parameter.plan_buy_total;//要改成plan_buy_total
            one.buy_succeed_min_percent = itr->template_parameter.buy_succeed_min_percent;
            one.need_raising       = itr->template_parameter.need_raising;

            one.create_time  = itr->status_expires.create_time;
            one.phase1_begin = itr->status_expires.phase1_begin; // 认购第1阶段开始时间
            one.phase1_end   = itr->status_expires.phase1_end; // 认购第1阶段结束时间
            one.phase2_begin = itr->status_expires.phase2_begin; // 认购第2阶段开始时间
            one.phase2_end   = itr->status_expires.phase2_end;
            one.settle_time  = itr->status_expires.settle_time;//token的结算时间


            one.guaranty_core_asset_amount = itr->template_parameter.guaranty_core_asset_amount;//抵押AGC数量


            //根据itr->statistics查找投票统计的动态信息
            token_statistics_object* token_statistics = (token_statistics_object*)_db.find_object(itr->statistics);
            if (token_statistics == NULL)//主题创建了，但是还没有投票
            {
                elog("subject statistics id ${id} is not found.", ("id", itr->statistics));
            }
            else
            {
                one.token_id                   = token_statistics->token_id;
                one.actual_buy_amount          = token_statistics->actual_buy_total;//所有参与的用户已经认购的用户资产数量，这个考虑一下怎么算
                one.actual_core_asset_total    = token_statistics->actual_core_asset_total;//所有参与的用户已经募集的AGC数量
                one.buyer_number               = token_statistics->buyer_number;
            }

            one.buy_count = 0;
            //查看当前账户是否投过票
            if ( condition.my_account == "" || condition.my_account == "null" )
            {
                elog("account name or id [${account}] is empty.", ("account",condition.my_account) );
                //one.subject_vote_id = optional<object_id_type>();
            }
            else
            {
                const account_object* account = nullptr;
                if (std::isdigit(condition.my_account[0]))
                      account = _db.find(fc::variant(condition.my_account).as<account_id_type>());
                else
                {
                      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
                      auto itr = idx.find(condition.my_account);//如果这里为字符串null的话会导致程序崩溃
                      if (itr != idx.end())
                            account = &*itr;
                }
                if (account != nullptr)
                {
                      //创建者也算是参与者
                    if(itr->issuer == account->id )
                    {
                        one.buy_count++;
                    }

                    const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_id>();
                    auto itr_buy = idx_buy.begin();

                    for(;itr_buy != idx_buy.end(); itr_buy++)
                    {
                        if(itr_buy->buyer == account->id && itr_buy->token_id == one.token_id)
                        {
                            one.buy_count++;
                            if (type == my_tokens_query)
                            {
                                one.my_participate.push_back(itr_buy->template_parameter);
                            }
                            else
                                break;
                        }
                    }
                }
            }

            //处理扩展字段
            TokenFillExtendField(itr->exts, condition.extra_query_fields, one.extend_field);
            if ( type == my_tokens_query )
            {
                if (one.buy_count)
                {
                    result.push_back(one);
                }
            }
            else
            {
                result.push_back(one);
            }
            
            if(result.size() == condition.limit) break;
        }
        return result;        
    }

    template<typename T>
    std::vector<token_brief> get_tokens_brief_template_by_global_negative(const token_query_condition &condition, query_token_type type, const string &ext="")const
    {
        std::vector<token_brief> result;
        if (condition.limit ==0) return {};

        const auto &idx = _db.get_index_type<token_index>().indices().get<T>();
        if(idx.size() == 0) return result;

        for(auto itr_test=idx.begin(); itr_test !=idx.end(); itr_test++)
        {
        	string issuer_name = _db.find(itr_test->issuer)->name;
        	elog("token object id is ${token_object_id},issuer is ${issuer}",("token_object_id",itr_test->id)("issuer", issuer_name));
        }

        //都是从大到小排序
        uint32_t count=0;
        auto itr = idx.end();
        itr--;
        for(;true;itr--)
        {
            if ( condition.status != "all" )
            {
                if ( condition.status == "end" && itr->status < token_object::settle_status ) 
                {
                	if(itr == idx.begin()) break;
                	continue;
                }
                if ( condition.status == "buy" && (itr->status >= token_object::settle_status || itr->status <=token_object::create_status) ) 
                {
                	if(itr == idx.begin()) break;
                	continue;
                }
            }
            
            //if(typeid(T).name() == "by_create_time")
            if (ext == "by_create_time")
            {
                if(itr->status_expires.create_time < condition.start_time || itr->status_expires.create_time > condition.end_time)
                {
                	if(itr == idx.begin()) break;
                    continue;
                }
            }
            //else if(typeid(T).name() == "by_end_time")
            else if(ext == "by_end_time")
            {
                if(itr->status_expires.settle_time < condition.start_time || itr->status_expires.settle_time > condition.end_time)
                {
                	if(itr == idx.begin()) break;
                    continue;
                }                
            }
            
            if(count+1 <= condition.start)
            {
                count++;
                if(itr == idx.begin()) break;
                continue;
            }

            token_brief one;

            if (itr->control == token_object::token_control::unavailable)
            {
            	if(itr == idx.begin()) break;
                continue;
            }
            else if (itr->control == token_object::token_control::description_forbidden)
            {
                one.brief       = "This token is description forbidden.";
                one.description = "This token is description forbidden.";
            }
            else
            {
                one.brief       = itr->template_parameter.brief;
                one.description = itr->template_parameter.description;
            }

            one.status             = itr->status;
            //根据id查询名字
            one.issuer             = _db.find(itr->issuer)->name;
            one.guaranty_credit    = itr->guaranty_credit;
            one.control            = itr->control;
            one.logo_url           = itr->template_parameter.logo_url;
            one.asset_name         = itr->template_parameter.asset_name;
            one.asset_symbol       = itr->template_parameter.asset_symbol;
            one.max_supply         = itr->template_parameter.max_supply;//token的最大供应量
            one.plan_buy_total     = itr->template_parameter.plan_buy_total;//要改成plan_buy_total
            one.buy_succeed_min_percent = itr->template_parameter.buy_succeed_min_percent;
            one.need_raising       = itr->template_parameter.need_raising;

            one.create_time  = itr->status_expires.create_time;
            one.phase1_begin = itr->status_expires.phase1_begin; // 认购第1阶段开始时间
            one.phase1_end   = itr->status_expires.phase1_end; // 认购第1阶段结束时间
            one.phase2_begin = itr->status_expires.phase2_begin; // 认购第2阶段开始时间
            one.phase2_end   = itr->status_expires.phase2_end;
            one.settle_time  = itr->status_expires.settle_time;//token的结算时间
            one.guaranty_core_asset_amount = itr->template_parameter.guaranty_core_asset_amount;//抵押AGC数量


            //根据itr->statistics查找投票统计的动态信息
            token_statistics_object* token_statistics = (token_statistics_object*)_db.find_object(itr->statistics);
            if (token_statistics == NULL)//主题创建了，但是还没有投票
            {
                elog("subject statistics id ${id} is not found.", ("id", itr->statistics));
            }
            else
            {
                elog("token id is ${token_id}",("token_id",token_statistics->token_id));
                one.token_id                   = token_statistics->token_id;
                one.actual_buy_amount          = token_statistics->actual_buy_total;//所有参与的用户已经认购的用户资产数量，这个考虑一下怎么算
                one.actual_core_asset_total    = token_statistics->actual_core_asset_total;//所有参与的用户已经募集的AGC数量
                one.buyer_number               = token_statistics->buyer_number;
            }

            one.buy_count = 0;
            //查看当前账户是否投过票
            if ( condition.my_account == "" || condition.my_account == "null" )
            {
                elog("account name or id [${account}] is empty.", ("account",condition.my_account) );
                //one.subject_vote_id = optional<object_id_type>();
            }
            else
            {
                const account_object* account = nullptr;
                if (std::isdigit(condition.my_account[0]))
                    account = _db.find(fc::variant(condition.my_account).as<account_id_type>());
                else
                {
                    const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
                    auto itr = idx.find(condition.my_account);//如果这里为字符串null的话会导致程序崩溃
                    if (itr != idx.end())
                        account = &*itr;
                }
                if (account != nullptr)
                {
                    //创建者也算是参与者
                    if(itr->issuer == account->id )
                    {
                        one.buy_count++;
                    }

                    const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_id>();
                    auto itr_buy = idx_buy.begin();

                    for(;itr_buy != idx_buy.end(); itr_buy++)
                    {
                        if(itr_buy->buyer == account->id && itr_buy->token_id == one.token_id)
                        {
                            one.buy_count++;
                            if (type == my_tokens_query)
                            {
                                one.my_participate.push_back(itr_buy->template_parameter);
                            }
                            else
                                break;
                        }
                    }
                }
            }

            //处理扩展字段
            TokenFillExtendField(itr->exts, condition.extra_query_fields, one.extend_field);
            if ( type == my_tokens_query )
            {
                if (one.buy_count)
                {
                    result.push_back(one);
                }
            }
            else
            {
                result.push_back(one);
            }

            if (itr == idx.begin()) break;
            if (result.size() == condition.limit) break;
        }  
        return result;  
    }
	//[end]
      //delay_transfer
      std::vector<delay_transfer_object_for_query> get_delay_transfer_by_from(uint32_t start, uint32_t limit, string from_name_or_id, uint8_t query_type = 0) const;
      std::vector<delay_transfer_object_for_query> get_delay_transfer_by_to(uint32_t start, uint32_t limit, string to_name_or_id, uint8_t query_type = 0) const;
      optional<delay_transfer_unexecuted_object> get_delay_transfer_unexecuted_asset_by_to( string to_name_or_id )const;


      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;

         if( !is_subscribed_to_item(i) )
         {
            idump((i));
            _subscribe_filter.insert( vec.data(), vec.size() );//(vecconst char*)&i, sizeof(i) );
         }
      }

      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;

         return _subscribe_filter.contains( i );
      }

      bool is_impacted_account( const flat_set<account_id_type>& accounts)
      {
         if( !_subscribed_accounts.size() || !accounts.size() )
            return false;

         return std::any_of(accounts.begin(), accounts.end(), [this](const account_id_type& account) {
            return _subscribed_accounts.find(account) != _subscribed_accounts.end();
         });
      }

      template<typename T>
      void enqueue_if_subscribed_to_market(const object* obj, market_queue_type& queue, bool full_object=true)
      {
         const T* order = dynamic_cast<const T*>(obj);
         FC_ASSERT( order != nullptr);

         auto market = order->get_market();

         auto sub = _market_subscriptions.find( market );
         if( sub != _market_subscriptions.end() ) {
            queue[market].emplace_back( full_object ? obj->to_variant() : fc::variant(obj->id) );
         }
      }

      void broadcast_updates( const vector<variant>& updates );
      void broadcast_market_updates( const market_queue_type& queue);
      void handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object);

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
      void on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
      void on_objects_removed(const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts);
      void on_applied_block();

      bool _notify_remove_create = false;
      mutable fc::bloom_filter _subscribe_filter;
      std::set<account_id_type> _subscribed_accounts;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection                                                                                           _new_connection;
      boost::signals2::scoped_connection                                                                                           _change_connection;
      boost::signals2::scoped_connection                                                                                           _removed_connection;
      boost::signals2::scoped_connection                                                                                           _applied_block_connection;
      boost::signals2::scoped_connection                                                                                           _pending_trx_connection;
      map< pair<asset_id_type,asset_id_type>, std::function<void(const variant&)> >      _market_subscriptions;
      graphene::chain::database&                                                                                                            _db;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( graphene::chain::database& db ):_db(db)
{
   wlog("creating database api ${x}", ("x",int64_t(this)) );
   _new_connection = _db.new_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_new(ids, impacted_accounts);
                                });
   _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_changed(ids, impacted_accounts);
                                });
   _removed_connection = _db.removed_objects.connect([this](const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts) {
                                on_objects_removed(ids, objs, impacted_accounts);
                                });
   _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });

   _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
                         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx) );
                      });
}

database_api_impl::~database_api_impl()
{
   elog("freeing database api ${x}", ("x",int64_t(this)) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Objects                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
{
   return my->get_objects( ids );
}

fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids)const
{
   if( _subscribe_callback )  {
      for( auto id : ids )
      {
         if( id.type() == operation_history_object_type && id.space() == protocol_ids ) continue;
         if( id.type() == impl_account_transaction_history_object_type && id.space() == implementation_ids ) continue;

         this->subscribe_to_item( id );
      }
   }

   fc::variants result;
   result.reserve(ids.size());

   std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                  [this](object_id_type id) -> fc::variant {
      if(auto obj = _db.find_object(id))
         return obj->to_variant();
      return {};
   });

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   my->set_subscribe_callback( cb, notify_remove_create );
}

void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool notify_remove_create )
{
   //edump((clear_filter));
   _subscribe_callback = cb;
   _notify_remove_create = notify_remove_create;
   _subscribed_accounts.clear();

   static fc::bloom_parameters param;
   param.projected_element_count    = 10000;
   param.false_positive_probability = 1.0/100;
   param.maximum_size = 1024*8*8*2;
   param.compute_optimal_parameters();
   _subscribe_filter = fc::bloom_filter(param);
}

void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   _pending_trx_callback = cb;
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   _block_applied_callback = cb;
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}

void database_api_impl::cancel_all_subscriptions()
{
   set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
   _market_subscriptions.clear();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   return my->get_block_header( block_num );
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if(result)
      return *result;
   return {};
}
map<uint32_t, optional<block_header>> database_api::get_block_header_batch(const vector<uint32_t> block_nums)const
{
   return my->get_block_header_batch( block_nums );
}

map<uint32_t, optional<block_header>> database_api_impl::get_block_header_batch(const vector<uint32_t> block_nums) const
{
   map<uint32_t, optional<block_header>> results;
   for (const uint32_t block_num : block_nums)
   {
      results[block_num] = get_block_header(block_num);
   }
   return results;
}

optional<signed_block> database_api::get_block(uint32_t block_num)const
{
   return my->get_block( block_num );
}

optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
{
   return _db.fetch_block_by_number(block_num);
}

processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block )const
{
   return my->get_transaction( block_num, trx_in_block );
}

optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
{
   try {
      return my->_db.get_recent_transaction( id );
   } catch ( ... ) {
      return optional<signed_transaction>();
   }
}

processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num)const
{
   auto opt_block = _db.fetch_block_by_number(block_num);
   FC_ASSERT( opt_block );
   FC_ASSERT( opt_block->transactions.size() > trx_num );
   return opt_block->transactions[trx_num];
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

chain_property_object database_api::get_chain_properties()const
{
   return my->get_chain_properties();
}

chain_property_object database_api_impl::get_chain_properties()const
{
   return _db.get(chain_property_id_type());
}

global_property_object database_api::get_global_properties()const
{
   return my->get_global_properties();
}

global_property_object database_api_impl::get_global_properties()const
{
   return _db.get(global_property_id_type());
}

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

fc::variant_object database_api_impl::get_config()const
{
   return graphene::chain::get_config();
}

chain_id_type database_api::get_chain_id()const
{
   return my->get_chain_id();
}

chain_id_type database_api_impl::get_chain_id()const
{
   return _db.get_chain_id();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
{
   return _db.get(dynamic_global_property_id_type());
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<vector<account_id_type>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   wdump( (keys) );
   vector< vector<account_id_type> > final_result;
   final_result.reserve(keys.size());

   for( auto& key : keys )
   {

      address a1( pts_address(key, false, 56) );
      address a2( pts_address(key, true, 56) );
      address a3( pts_address(key, false, 0)  );
      address a4( pts_address(key, true, 0)  );
      address a5( key );

      subscribe_to_item( key );
      subscribe_to_item( a1 );
      subscribe_to_item( a2 );
      subscribe_to_item( a3 );
      subscribe_to_item( a4 );
      subscribe_to_item( a5 );

      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_key_memberships.find(key);
      vector<account_id_type> result;

      for( auto& a : {a1,a2,a3,a4,a5} )
      {
          auto itr = refs.account_to_address_memberships.find(a);
          if( itr != refs.account_to_address_memberships.end() )
          {
             result.reserve( itr->second.size() );
             for( auto item : itr->second )
             {
                wdump((a)(item)(item(_db).name));
                result.push_back(item);
             }
          }
      }

      if( itr != refs.account_to_key_memberships.end() )
      {
         result.reserve( itr->second.size() );
         for( auto item : itr->second ) result.push_back(item);
      }
      final_result.emplace_back( std::move(result) );
   }

   for( auto i : final_result )
      subscribe_to_item(i);

   return final_result;
}

bool database_api::is_public_key_registered(string public_key) const
{
    return my->is_public_key_registered(public_key);
}

bool database_api_impl::is_public_key_registered(string public_key) const
{
    // Short-circuit
    if (public_key.empty()) {
        return false;
    }

    // Search among all keys using an existing map of *current* account keys
    public_key_type key;
    try {
        key = public_key_type(public_key);
    } catch ( ... ) {
        // An invalid public key was detected
        return false;
    }
    const auto& idx = _db.get_index_type<account_index>();
    const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
    const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
    auto itr = refs.account_to_key_memberships.find(key);
    bool is_known = itr != refs.account_to_key_memberships.end();

    return is_known;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids)const
{
   return my->get_accounts( account_ids );
}

vector<optional<account_object>> database_api_impl::get_accounts(const vector<account_id_type>& account_ids)const
{
   vector<optional<account_object>> result; result.reserve(account_ids.size());
   std::transform(account_ids.begin(), account_ids.end(), std::back_inserter(result),
                  [this](account_id_type id) -> optional<account_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

std::map<string,full_account> database_api::get_full_accounts( const vector<string>& names_or_ids, bool subscribe )
{
   return my->get_full_accounts( names_or_ids, subscribe );
}

std::map<std::string, full_account> database_api_impl::get_full_accounts( const vector<std::string>& names_or_ids, bool subscribe)
{
   idump((names_or_ids));
   std::map<std::string, full_account> results;

   for (const std::string& account_name_or_id : names_or_ids)
   {
      const account_object* account = nullptr;
      if (std::isdigit(account_name_or_id[0]))
         account = _db.find(fc::variant(account_name_or_id).as<account_id_type>());
      else
      {
         const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(account_name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }
      if (account == nullptr)
         continue;

      if( subscribe )
      {
         FC_ASSERT( std::distance(_subscribed_accounts.begin(), _subscribed_accounts.end()) <= 100 );
         _subscribed_accounts.insert( account->get_id() );
         subscribe_to_item( account->id );
      }

      // fc::mutable_variant_object full_account;
      full_account acnt;
      acnt.account = *account;
      acnt.statistics = account->statistics(_db);
      acnt.registrar_name = account->registrar(_db).name;
      acnt.referrer_name = account->referrer(_db).name;
      acnt.lifetime_referrer_name = account->lifetime_referrer(_db).name;
      acnt.votes = lookup_vote_ids( vector<vote_id_type>(account->options.votes.begin(),account->options.votes.end()) );

      // Add the account itself, its statistics object, cashback balance, and referral account names
      /*
      full_account("account", *account)("statistics", account->statistics(_db))
            ("registrar_name", account->registrar(_db).name)("referrer_name", account->referrer(_db).name)
            ("lifetime_referrer_name", account->lifetime_referrer(_db).name);
            */
      if (account->cashback_vb)
      {
         acnt.cashback_balance = account->cashback_balance(_db);
      }
      // Add the account's proposals
      const auto& proposal_idx = _db.get_index_type<proposal_index>();
      const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
      const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
      auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
      if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
      {
         acnt.proposals.reserve( required_approvals_itr->second.size() );
         for( auto proposal_id : required_approvals_itr->second )
            acnt.proposals.push_back( proposal_id(_db) );
      }


      // Add the account's balances
      auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
      //vector<account_balance_object> balances;
      std::for_each(balance_range.first, balance_range.second,
                    [&acnt](const account_balance_object& balance) {
                       acnt.balances.emplace_back(balance);
                    });

      // Add the account's vesting balances
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&acnt](const vesting_balance_object& balance) {
                       acnt.vesting_balances.emplace_back(balance);
                    });

      // Add the account's orders
      auto order_range = _db.get_index_type<limit_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(order_range.first, order_range.second,
                    [&acnt] (const limit_order_object& order) {
                       acnt.limit_orders.emplace_back(order);
                    });
      auto call_range = _db.get_index_type<call_order_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(call_range.first, call_range.second,
                    [&acnt] (const call_order_object& call) {
                       acnt.call_orders.emplace_back(call);
                    });
      auto settle_range = _db.get_index_type<force_settlement_index>().indices().get<by_account>().equal_range(account->id);
      std::for_each(settle_range.first, settle_range.second,
                    [&acnt] (const force_settlement_object& settle) {
                       acnt.settle_orders.emplace_back(settle);
                    });

      // get assets issued by user
      auto asset_range = _db.get_index_type<asset_index>().indices().get<by_issuer>().equal_range(account->id);
      std::for_each(asset_range.first, asset_range.second,
                    [&acnt] (const asset_object& asset) {
                       acnt.assets.emplace_back(asset.id);
                    });

      // get withdraws permissions
      auto withdraw_range = _db.get_index_type<withdraw_permission_index>().indices().get<by_from>().equal_range(account->id);
      std::for_each(withdraw_range.first, withdraw_range.second,
                    [&acnt] (const withdraw_permission_object& withdraw) {
                       acnt.withdraws.emplace_back(withdraw);
                    });


      results[account_name_or_id] = acnt;
   }
   return results;
}

optional<account_object> database_api::get_account_by_name( string name )const
{
   return my->get_account_by_name( name );
}

optional<account_object> database_api_impl::get_account_by_name( string name )const
{
   const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = idx.find(name);
   if (itr != idx.end())
      return *itr;
   return optional<account_object>();
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->get_account_references( account_id );
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->lookup_account_names( account_names );
}

vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<optional<account_object> > result;
   result.reserve(account_names.size());
   std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                  [&accounts_by_name](const string& name) -> optional<account_object> {
      auto itr = accounts_by_name.find(name);
      return itr == accounts_by_name.end()? optional<account_object>() : *itr;
   });
   return result;
}

map<string,account_id_type> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts( lower_bound_name, limit );
}

map<string,account_id_type> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   map<string,account_id_type> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(make_pair(itr->name, itr->get_id()));
      if( limit == 1 )
         subscribe_to_item( itr->get_id() );
   }

   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index_type<account_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Balances                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<asset> database_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
{
   return my->get_account_balances( id, assets );
}

vector<asset> database_api_impl::get_account_balances(account_id_type acnt, const flat_set<asset_id_type>& assets)const
{
   vector<asset> result;
   if (assets.empty())
   {
      // if the caller passes in an empty list of assets, return balances for all assets the account owns
      const account_balance_index& balance_index = _db.get_index_type<account_balance_index>();
      auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acnt));
      for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
         result.push_back(asset(balance.get_balance()));
   }
   else
   {
      result.reserve(assets.size());

      std::transform(assets.begin(), assets.end(), std::back_inserter(result),
                     [this, acnt](asset_id_type id) { return _db.get_balance(acnt, id); });
   }

   return result;
}

vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const
{
   return my->get_named_account_balances( name, assets );
}

vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets) const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   auto itr = accounts_by_name.find(name);
   FC_ASSERT( itr != accounts_by_name.end() );
   return get_account_balances(itr->get_id(), assets);
}

vector<balance_object> database_api::get_balance_objects( const vector<address>& addrs )const
{
   return my->get_balance_objects( addrs );
}

vector<balance_object> database_api_impl::get_balance_objects( const vector<address>& addrs )const
{
   try
   {
      const auto& bal_idx = _db.get_index_type<balance_index>();
      const auto& by_owner_idx = bal_idx.indices().get<by_owner>();

      vector<balance_object> result;

      for( const auto& owner : addrs )
      {
         subscribe_to_item( owner );
         auto itr = by_owner_idx.lower_bound( boost::make_tuple( owner, asset_id_type(0) ) );
         while( itr != by_owner_idx.end() && itr->owner == owner )
         {
            result.push_back( *itr );
            ++itr;
         }
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (addrs) )
}

vector<asset> database_api::get_vested_balances( const vector<balance_id_type>& objs )const
{
   return my->get_vested_balances( objs );
}

vector<asset> database_api_impl::get_vested_balances( const vector<balance_id_type>& objs )const
{
   try
   {
      vector<asset> result;
      result.reserve( objs.size() );
      auto now = _db.head_block_time();
      for( auto obj : objs )
         result.push_back( obj(_db).available( now ) );
      return result;
   } FC_CAPTURE_AND_RETHROW( (objs) )
}

vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
{
   return my->get_vesting_balances( account_id );
}

vector<vesting_balance_object> database_api_impl::get_vesting_balances( account_id_type account_id )const
{
   try
   {
      vector<vesting_balance_object> result;
      auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account_id);
      std::for_each(vesting_range.first, vesting_range.second,
                    [&result](const vesting_balance_object& balance) {
                       result.emplace_back(balance);
                    });
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (account_id) );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Assets                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<asset_object>> database_api::get_assets(const vector<asset_id_type>& asset_ids)const
{
   return my->get_assets( asset_ids );
}

vector<optional<asset_object>> database_api_impl::get_assets(const vector<asset_id_type>& asset_ids)const
{
   vector<optional<asset_object>> result; result.reserve(asset_ids.size());
   std::transform(asset_ids.begin(), asset_ids.end(), std::back_inserter(result),
                  [this](asset_id_type id) -> optional<asset_object> {
      if(auto o = _db.find(id))
      {
         subscribe_to_item( id );
         return *o;
      }
      return {};
   });
   return result;
}

vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   return my->list_assets( lower_bound_symbol, limit );
}

vector<asset_object> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
{
   FC_ASSERT( limit <= 100 );
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<asset_object> result;
   result.reserve(limit);

   auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

   if( lower_bound_symbol == "" )
      itr = assets_by_symbol.begin();

   while(limit-- && itr != assets_by_symbol.end())
      result.emplace_back(*itr++);

   return result;
}

vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   return my->lookup_asset_symbols( symbols_or_ids );
}

vector<optional<asset_object>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
{
   const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
   vector<optional<asset_object> > result;
   result.reserve(symbols_or_ids.size());
   std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                  [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object> {
      if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
      {
         auto ptr = _db.find(variant(symbol_or_id).as<asset_id_type>());
         return ptr == nullptr? optional<asset_object>() : *ptr;
      }
      auto itr = assets_by_symbol.find(symbol_or_id);
      return itr == assets_by_symbol.end()? optional<asset_object>() : *itr;
   });
   return result;
}



//////////////////////////////////////////////////////////////////////
//                                                                  //
// module config                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

module_cfg_object database_api::get_module_cfg(const string& module_name)const
{
   return my->get_module_cfg(module_name);
}
module_cfg_object database_api_impl::get_module_cfg(const string& module_name)const
{
   return _db.get_module_cfg(module_name);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Markets / feeds                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<limit_order_object> database_api::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   return my->get_limit_orders( a, b, limit );
}

/**
 *  @return the limit orders for both sides of the book for the two assets specified up to limit number on each side.
 */
vector<limit_order_object> database_api_impl::get_limit_orders(asset_id_type a, asset_id_type b, uint32_t limit)const
{
   const auto& limit_order_idx = _db.get_index_type<limit_order_index>();
   const auto& limit_price_idx = limit_order_idx.indices().get<by_price>();

   vector<limit_order_object> result;

   uint32_t count = 0;
   auto limit_itr = limit_price_idx.lower_bound(price::max(a,b));
   auto limit_end = limit_price_idx.upper_bound(price::min(a,b));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }
   count = 0;
   limit_itr = limit_price_idx.lower_bound(price::max(b,a));
   limit_end = limit_price_idx.upper_bound(price::min(b,a));
   while(limit_itr != limit_end && count < limit)
   {
      result.push_back(*limit_itr);
      ++limit_itr;
      ++count;
   }

   return result;
}

vector<call_order_object> database_api::get_call_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_call_orders( a, limit );
}

vector<call_order_object> database_api_impl::get_call_orders(asset_id_type a, uint32_t limit)const
{
   const auto& call_index = _db.get_index_type<call_order_index>().indices().get<by_price>();
   const asset_object& mia = _db.get(a);
   price index_price = price::min(mia.bitasset_data(_db).options.short_backing_asset, mia.get_id());

   return vector<call_order_object>(call_index.lower_bound(index_price.min()),
                                    call_index.lower_bound(index_price.max()));
}

vector<force_settlement_object> database_api::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   return my->get_settle_orders( a, limit );
}

vector<force_settlement_object> database_api_impl::get_settle_orders(asset_id_type a, uint32_t limit)const
{
   const auto& settle_index = _db.get_index_type<force_settlement_index>().indices().get<by_expiration>();
   const asset_object& mia = _db.get(a);
   return vector<force_settlement_object>(settle_index.lower_bound(mia.get_id()),
                                          settle_index.upper_bound(mia.get_id()));
}

vector<call_order_object> database_api::get_margin_positions( const account_id_type& id )const
{
   return my->get_margin_positions( id );
}

vector<call_order_object> database_api_impl::get_margin_positions( const account_id_type& id )const
{
   try
   {
      const auto& idx = _db.get_index_type<call_order_index>();
      const auto& aidx = idx.indices().get<by_account>();
      auto start = aidx.lower_bound( boost::make_tuple( id, asset_id_type(0) ) );
      auto end = aidx.lower_bound( boost::make_tuple( id+1, asset_id_type(0) ) );
      vector<call_order_object> result;
      while( start != end )
      {
         result.push_back(*start);
         ++start;
      }
      return result;
   } FC_CAPTURE_AND_RETHROW( (id) )
}

void database_api::subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b)
{
   my->subscribe_to_market( callback, a, b );
}

void database_api_impl::subscribe_to_market(std::function<void(const variant&)> callback, asset_id_type a, asset_id_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions[ std::make_pair(a,b) ] = callback;
}

void database_api::unsubscribe_from_market(asset_id_type a, asset_id_type b)
{
   my->unsubscribe_from_market( a, b );
}

void database_api_impl::unsubscribe_from_market(asset_id_type a, asset_id_type b)
{
   if(a > b) std::swap(a,b);
   FC_ASSERT(a != b);
   _market_subscriptions.erase(std::make_pair(a,b));
}

market_ticker database_api::get_ticker( const string& base, const string& quote )const
{
    return my->get_ticker( base, quote );
}

market_ticker database_api_impl::get_ticker( const string& base, const string& quote )const
{
    const auto assets = lookup_asset_symbols( {base, quote} );
    FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
    FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

    market_ticker result;
    result.base = base;
    result.quote = quote;
    result.latest = 0;
    result.lowest_ask = 0;
    result.highest_bid = 0;
    result.percent_change = 0;
    result.base_volume = 0;
    result.quote_volume = 0;

    try {
        const fc::time_point_sec now = fc::time_point::now();
        const fc::time_point_sec yesterday = fc::time_point_sec( now.sec_since_epoch() - 86400 );
        const auto batch_size = 100;

        vector<market_trade> trades = get_trade_history( base, quote, now, yesterday, batch_size );
        if( !trades.empty() )
        {
            result.latest = trades[0].price;

            while( !trades.empty() )
            {
                for( const market_trade& t: trades )
                {
                    result.base_volume += t.value;
                    result.quote_volume += t.amount;
                }

                trades = get_trade_history( base, quote, trades.back().date, yesterday, batch_size );
            }

            const auto last_trade_yesterday = get_trade_history( base, quote, yesterday, fc::time_point_sec(), 1 );
            if( !last_trade_yesterday.empty() )
            {
                const auto price_yesterday = last_trade_yesterday[0].price;
                result.percent_change = ( (result.latest / price_yesterday) - 1 ) * 100;
            }
        }
        else
        {
            const auto last_trade = get_trade_history( base, quote, now, fc::time_point_sec(), 1 );
            if( !last_trade.empty() )
                result.latest = last_trade[0].price;
        }

        const auto orders = get_order_book( base, quote, 1 );
        if( !orders.asks.empty() ) result.lowest_ask = orders.asks[0].price;
        if( !orders.bids.empty() ) result.highest_bid = orders.bids[0].price;
    } FC_CAPTURE_AND_RETHROW( (base)(quote) )

    return result;
}

market_volume database_api::get_24_volume( const string& base, const string& quote )const
{
    return my->get_24_volume( base, quote );
}

market_volume database_api_impl::get_24_volume( const string& base, const string& quote )const
{
    const auto ticker = get_ticker( base, quote );

    market_volume result;
    result.base = ticker.base;
    result.quote = ticker.quote;
    result.base_volume = ticker.base_volume;
    result.quote_volume = ticker.quote_volume;

    return result;
}

order_book database_api::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   return my->get_order_book( base, quote, limit);
}

order_book database_api_impl::get_order_book( const string& base, const string& quote, unsigned limit )const
{
   using boost::multiprecision::uint128_t;
   FC_ASSERT( limit <= 50 );

   order_book result;
   result.base = base;
   result.quote = quote;

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;
   auto orders = get_limit_orders( base_id, quote_id, limit );


   auto asset_to_real = [&]( const asset& a, int p ) { return double(a.amount.value)/pow( 10, p ); };
   auto price_to_real = [&]( const price& p )
   {
      if( p.base.asset_id == base_id )
         return asset_to_real( p.base, assets[0]->precision ) / asset_to_real( p.quote, assets[1]->precision );
      else
         return asset_to_real( p.quote, assets[0]->precision ) / asset_to_real( p.base, assets[1]->precision );
   };

   for( const auto& o : orders )
   {
      if( o.sell_price.base.asset_id == base_id )
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( share_type( ( uint128_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[1]->precision );
         ord.base = asset_to_real( o.for_sale, assets[0]->precision );
         result.bids.push_back( ord );
      }
      else
      {
         order ord;
         ord.price = price_to_real( o.sell_price );
         ord.quote = asset_to_real( o.for_sale, assets[1]->precision );
         ord.base = asset_to_real( share_type( ( uint128_t( o.for_sale.value ) * o.sell_price.quote.amount.value ) / o.sell_price.base.amount.value ), assets[0]->precision );
         result.asks.push_back( ord );
      }
   }

   return result;
}

vector<market_trade> database_api::get_trade_history( const string& base,
                                                      const string& quote,
                                                      fc::time_point_sec start,
                                                      fc::time_point_sec stop,
                                                      unsigned limit )const
{
   return my->get_trade_history( base, quote, start, stop, limit );
}

vector<market_trade> database_api_impl::get_trade_history( const string& base,
                                                           const string& quote,
                                                           fc::time_point_sec start,
                                                           fc::time_point_sec stop,
                                                           unsigned limit )const
{
   FC_ASSERT( limit <= 100 );

   auto assets = lookup_asset_symbols( {base, quote} );
   FC_ASSERT( assets[0], "Invalid base asset symbol: ${s}", ("s",base) );
   FC_ASSERT( assets[1], "Invalid quote asset symbol: ${s}", ("s",quote) );

   auto base_id = assets[0]->id;
   auto quote_id = assets[1]->id;

   if( base_id > quote_id ) std::swap( base_id, quote_id );
   const auto& history_idx = _db.get_index_type<graphene::market_history::history_index>().indices().get<by_key>();
   history_key hkey;
   hkey.base = base_id;
   hkey.quote = quote_id;
   hkey.sequence = std::numeric_limits<int64_t>::min();

   auto price_to_real = [&]( const share_type a, int p ) { return double( a.value ) / pow( 10, p ); };

   if ( start.sec_since_epoch() == 0 )
      start = fc::time_point_sec( fc::time_point::now() );

   uint32_t count = 0;
   auto itr = history_idx.lower_bound( hkey );
   vector<market_trade> result;

   while( itr != history_idx.end() && count < limit && !( itr->key.base != base_id || itr->key.quote != quote_id || itr->time < stop ) )
   {
      if( itr->time < start )
      {
         market_trade trade;

         if( assets[0]->id == itr->op.receives.asset_id )
         {
            trade.amount = price_to_real( itr->op.pays.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.receives.amount, assets[0]->precision );
         }
         else
         {
            trade.amount = price_to_real( itr->op.receives.amount, assets[1]->precision );
            trade.value = price_to_real( itr->op.pays.amount, assets[0]->precision );
         }

         trade.date = itr->time;
         trade.price = trade.value / trade.amount;

         result.push_back( trade );
         ++count;
      }

      // Trades are tracked in each direction.
      ++itr;
      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_object>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   return my->get_witnesses( witness_ids );
}

vector<worker_object> database_api::get_workers_by_account(account_id_type account)const
{
    const auto& idx = my->_db.get_index_type<worker_index>().indices().get<by_account>();
    auto itr = idx.find(account);
    vector<worker_object> result;

    if( itr != idx.end() && itr->worker_account == account )
    {
       result.emplace_back( *itr );
       ++itr;
    }

    return result;
}


vector<optional<witness_object>> database_api_impl::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   vector<optional<witness_object>> result; result.reserve(witness_ids.size());
   std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                  [this](witness_id_type id) -> optional<witness_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<witness_object> database_api::get_witness_by_account(account_id_type account)const
{
   return my->get_witness_by_account( account );
}

fc::optional<witness_object> database_api_impl::get_witness_by_account(account_id_type account) const
{
   const auto& idx = _db.get_index_type<witness_index>().indices().get<by_account>();
   auto itr = idx.find(account);
   if( itr != idx.end() )
      return *itr;
   return {};
}

map<string, witness_id_type> database_api::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_witness_accounts( lower_bound_name, limit );
}

map<string, witness_id_type> database_api_impl::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& witnesses_by_id = _db.get_index_type<witness_index>().indices().get<by_id>();

   // we want to order witnesses by account name, but that name is in the account object
   // so the witness_index doesn't have a quick way to access it.
   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of witnesses to be few and the frequency of calls to be rare
   std::map<std::string, witness_id_type> witnesses_by_account_name;
   for (const witness_object& witness : witnesses_by_id)
       if (auto account_iter = _db.find(witness.witness_account))
           if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               witnesses_by_account_name.insert(std::make_pair(account_iter->name, witness.id));

   auto end_iter = witnesses_by_account_name.begin();
   while (end_iter != witnesses_by_account_name.end() && limit--)
       ++end_iter;
   witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
   return witnesses_by_account_name;
}

uint64_t database_api::get_witness_count()const
{
   return my->get_witness_count();
}

uint64_t database_api_impl::get_witness_count()const
{
   return _db.get_index_type<witness_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Committee members                                                //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<committee_member_object>> database_api::get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const
{
   return my->get_committee_members( committee_member_ids );
}

vector<optional<committee_member_object>> database_api_impl::get_committee_members(const vector<committee_member_id_type>& committee_member_ids)const
{
   vector<optional<committee_member_object>> result; result.reserve(committee_member_ids.size());
   std::transform(committee_member_ids.begin(), committee_member_ids.end(), std::back_inserter(result),
                  [this](committee_member_id_type id) -> optional<committee_member_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<committee_member_object> database_api::get_committee_member_by_account(account_id_type account)const
{
   return my->get_committee_member_by_account( account );
}

fc::optional<committee_member_object> database_api_impl::get_committee_member_by_account(account_id_type account) const
{
   const auto& idx = _db.get_index_type<committee_member_index>().indices().get<by_account>();
   auto itr = idx.find(account);
   if( itr != idx.end() )
      return *itr;
   return {};
}

map<string, committee_member_id_type> database_api::lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_committee_member_accounts( lower_bound_name, limit );
}

map<string, committee_member_id_type> database_api_impl::lookup_committee_member_accounts(const string& lower_bound_name, uint32_t limit)const
{
   FC_ASSERT( limit <= 1000 );
   const auto& committee_members_by_id = _db.get_index_type<committee_member_index>().indices().get<by_id>();

   // we want to order committee_members by account name, but that name is in the account object
   // so the committee_member_index doesn't have a quick way to access it.
   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of committee_members to be few and the frequency of calls to be rare
   std::map<std::string, committee_member_id_type> committee_members_by_account_name;
   for (const committee_member_object& committee_member : committee_members_by_id)
       if (auto account_iter = _db.find(committee_member.committee_member_account))
           if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               committee_members_by_account_name.insert(std::make_pair(account_iter->name, committee_member.id));

   auto end_iter = committee_members_by_account_name.begin();
   while (end_iter != committee_members_by_account_name.end() && limit--)
       ++end_iter;
   committee_members_by_account_name.erase(end_iter, committee_members_by_account_name.end());
   return committee_members_by_account_name;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Votes                                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<variant> database_api::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   return my->lookup_vote_ids( votes );
}

vector<variant> database_api_impl::lookup_vote_ids( const vector<vote_id_type>& votes )const
{
   FC_ASSERT( votes.size() < 1000, "Only 1000 votes can be queried at a time" );

   const auto& witness_idx = _db.get_index_type<witness_index>().indices().get<by_vote_id>();
   const auto& committee_idx = _db.get_index_type<committee_member_index>().indices().get<by_vote_id>();
   const auto& for_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_for>();
   const auto& against_worker_idx = _db.get_index_type<worker_index>().indices().get<by_vote_against>();

   vector<variant> result;
   result.reserve( votes.size() );
   for( auto id : votes )
   {
      switch( id.type() )
      {
         case vote_id_type::committee:
         {
            auto itr = committee_idx.find( id );
            if( itr != committee_idx.end() )
               result.emplace_back( variant( *itr ) );
            else
               result.emplace_back( variant() );
            break;
         }
         case vote_id_type::witness:
         {
            auto itr = witness_idx.find( id );
            if( itr != witness_idx.end() )
               result.emplace_back( variant( *itr ) );
            else
               result.emplace_back( variant() );
            break;
         }
         case vote_id_type::worker:
         {
            auto itr = for_worker_idx.find( id );
            if( itr != for_worker_idx.end() ) {
               result.emplace_back( variant( *itr ) );
            }
            else {
               auto itr = against_worker_idx.find( id );
               if( itr != against_worker_idx.end() ) {
                  result.emplace_back( variant( *itr ) );
               }
               else {
                  result.emplace_back( variant() );
               }
            }
            break;
         }
         case vote_id_type::VOTE_TYPE_COUNT: break; // supress unused enum value warnings
      }
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->get_transaction_hex( trx );
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   wdump((trx)(available_keys));
   auto result = trx.get_required_signatures( _db.get_chain_id(),
                                       available_keys,
                                       [&]( account_id_type id ){ return &id(_db).active; },
                                       [&]( account_id_type id ){ return &id(_db).owner; },
                                       _db.get_global_properties().parameters.max_authority_depth );
   wdump((result));
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}
set<address> database_api::get_potential_address_signatures( const signed_transaction& trx )const
{
   return my->get_potential_address_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   wdump((trx));
   set<public_key_type> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );

   wdump((result));
   return result;
}

set<address> database_api_impl::get_potential_address_signatures( const signed_transaction& trx )const
{
   set<address> result;
   trx.get_required_signatures(
      _db.get_chain_id(),
      flat_set<public_key_type>(),
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).active;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      [&]( account_id_type id )
      {
         const auto& auth = id(_db).owner;
         for( const auto& k : auth.get_addresses() )
            result.insert(k);
         return &auth;
      },
      _db.get_global_properties().parameters.max_authority_depth
   );
   return result;
}

bool database_api::verify_authority( const signed_transaction& trx )const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( _db.get_chain_id(),
                         [&]( account_id_type id ){ return &id(_db).active; },
                         [&]( account_id_type id ){ return &id(_db).owner; },
                          _db.get_global_properties().parameters.max_authority_depth );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name_or_id.size() > 0);
   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0]))
      account = _db.find(fc::variant(name_or_id).as<account_id_type>());
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   FC_ASSERT( account, "no such account" );


   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->id;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

processed_transaction database_api::validate_transaction( const signed_transaction& trx )const
{
   return my->validate_transaction( trx );
}

processed_transaction database_api_impl::validate_transaction( const signed_transaction& trx )const
{
   return _db.validate_transaction(trx);
}

vector< fc::variant > database_api::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   return my->get_required_fees( ops, id );
}

/**
 * Container method for mutually recursive functions used to
 * implement get_required_fees() with potentially nested proposals.
 */
struct get_required_fees_helper
{
   get_required_fees_helper(
      const fee_schedule& _current_fee_schedule,
      const price& _core_exchange_rate,
      uint32_t _max_recursion
      )
      : current_fee_schedule(_current_fee_schedule),
        core_exchange_rate(_core_exchange_rate),
        max_recursion(_max_recursion)
   {}

   fc::variant set_op_fees( operation& op )
   {
      if( op.which() == operation::tag<proposal_create_operation>::value )
      {
         return set_proposal_create_op_fees( op );
      }
      else
      {
         asset fee = current_fee_schedule.set_fee( op, core_exchange_rate );
         fc::variant result;
         fc::to_variant( fee, result );
         return result;
      }
   }

   fc::variant set_proposal_create_op_fees( operation& proposal_create_op )
   {
      proposal_create_operation& op = proposal_create_op.get<proposal_create_operation>();
      std::pair< asset, fc::variants > result;
      for( op_wrapper& prop_op : op.proposed_ops )
      {
         FC_ASSERT( current_recursion < max_recursion );
         ++current_recursion;
         result.second.push_back( set_op_fees( prop_op.op ) );
         --current_recursion;
      }
      // we need to do this on the boxed version, which is why we use
      // two mutually recursive functions instead of a visitor
      result.first = current_fee_schedule.set_fee( proposal_create_op, core_exchange_rate );
      fc::variant vresult;
      fc::to_variant( result, vresult );
      return vresult;
   }

   const fee_schedule& current_fee_schedule;
   const price& core_exchange_rate;
   uint32_t max_recursion;
   uint32_t current_recursion = 0;
};

vector< fc::variant > database_api_impl::get_required_fees( const vector<operation>& ops, asset_id_type id )const
{
   vector< operation > _ops = ops;
   //
   // we copy the ops because we need to mutate an operation to reliably
   // determine its fee, see #435
   //

   vector< fc::variant > result;
   result.reserve(ops.size());
   const asset_object& a = id(_db);
   get_required_fees_helper helper(
      _db.current_fee_schedule(),
      a.options.core_exchange_rate,
      GET_REQUIRED_FEES_MAX_RECURSION );
   for( operation& op : _ops )
   {
      result.push_back( helper.set_op_fees( op ) );
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Proposed transactions                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<proposal_object> database_api::get_proposed_transactions( account_id_type id )const
{
   return my->get_proposed_transactions( id );
}

/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposed_transactions( account_id_type id )const
{
   const auto& idx = _db.get_index_type<proposal_index>();
   vector<proposal_object> result;

   idx.inspect_all_objects( [&](const object& obj){
           const proposal_object& p = static_cast<const proposal_object&>(obj);
           if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
              result.push_back(p);
           else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
              result.push_back(p);
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blinded balances                                                 //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<blinded_balance_object> database_api::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   return my->get_blinded_balances( commitments );
}

vector<blinded_balance_object> database_api_impl::get_blinded_balances( const flat_set<commitment_type>& commitments )const
{
   vector<blinded_balance_object> result; result.reserve(commitments.size());
   const auto& bal_idx = _db.get_index_type<blinded_balance_index>();
   const auto& by_commitment_idx = bal_idx.indices().get<by_commitment>();
   for( const auto& c : commitments )
   {
      auto itr = by_commitment_idx.find( c );
      if( itr != by_commitment_idx.end() )
         result.push_back( *itr );
   }
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Private methods                                                  //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api_impl::broadcast_updates( const vector<variant>& updates )
{
   if( updates.size() && _subscribe_callback ) {
      auto capture_this = shared_from_this();
      fc::async([capture_this,updates](){
          if(capture_this->_subscribe_callback)
            capture_this->_subscribe_callback( fc::variant(updates) );
      });
   }
}

void database_api_impl::broadcast_market_updates( const market_queue_type& queue)
{
   if( queue.size() )
   {
      auto capture_this = shared_from_this();
      fc::async([capture_this, this, queue](){
          for( const auto& item : queue )
          {
            auto sub = _market_subscriptions.find(item.first);
            if( sub != _market_subscriptions.end() )
                sub->second( fc::variant(item.second ) );
          }
      });
   }
}

void database_api_impl::on_objects_removed( const vector<object_id_type>& ids, const vector<const object*>& objs, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, false, ids, impacted_accounts,
      [objs](object_id_type id) -> const object* {
         auto it = std::find_if(
               objs.begin(), objs.end(),
               [id](const object* o) {return o != nullptr && o->id == id;});

         if (it != objs.end())
            return *it;

         return nullptr;
      }
   );
}

void database_api_impl::on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(_notify_remove_create, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
{
   handle_object_changed(false, true, ids, impacted_accounts,
      std::bind(&object_database::find_object, &_db, std::placeholders::_1)
   );
}

void database_api_impl::handle_object_changed(bool force_notify, bool full_object, const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts, std::function<const object*(object_id_type id)> find_object)
{
   if( _subscribe_callback )
   {
      vector<variant> updates;

      for(auto id : ids)
      {
         if( force_notify || is_subscribed_to_item(id) || is_impacted_account(impacted_accounts) )
         {
            if( full_object )
            {
               auto obj = find_object(id);
               if( obj )
               {
                  updates.emplace_back( obj->to_variant() );
               }
            }
            else
            {
               updates.emplace_back( id );
            }
         }
      }

      broadcast_updates(updates);
   }

   if( _market_subscriptions.size() )
   {
      market_queue_type broadcast_queue;

      for(auto id : ids)
      {
         if( id.is<call_order_object>() )
         {
            enqueue_if_subscribed_to_market<call_order_object>( find_object(id), broadcast_queue, full_object );
         }
         else if( id.is<limit_order_object>() )
         {
            enqueue_if_subscribed_to_market<limit_order_object>( find_object(id), broadcast_queue, full_object );
         }
      }

      broadcast_market_updates(broadcast_queue);
   }
}

/** note: this method cannot yield because it is called in the middle of
 * apply a block.
 */
void database_api_impl::on_applied_block()
{
   if (_block_applied_callback)
   {
      auto capture_this = shared_from_this();
      block_id_type block_id = _db.head_block_id();
      fc::async([this,capture_this,block_id](){
         _block_applied_callback(fc::variant(block_id));
      });
   }

   if(_market_subscriptions.size() == 0)
      return;

   const auto& ops = _db.get_applied_operations();
   map< std::pair<asset_id_type,asset_id_type>, vector<pair<operation, operation_result>> > subscribed_markets_ops;
   for(const optional< operation_history_object >& o_op : ops)
   {
      if( !o_op.valid() )
         continue;
      const operation_history_object& op = *o_op;

      std::pair<asset_id_type,asset_id_type> market;
      switch(op.op.which())
      {
         /*  This is sent via the object_changed callback
         case operation::tag<limit_order_create_operation>::value:
            market = op.op.get<limit_order_create_operation>().get_market();
            break;
         */
         case operation::tag<fill_order_operation>::value:
            market = op.op.get<fill_order_operation>().get_market();
            break;
            /*
         case operation::tag<limit_order_cancel_operation>::value:
         */
         default: break;
      }
      if(_market_subscriptions.count(market))
         subscribed_markets_ops[market].push_back(std::make_pair(op.op, op.result));
   }
   /// we need to ensure the database_api is not deleted for the life of the async operation
   auto capture_this = shared_from_this();
   fc::async([this,capture_this,subscribed_markets_ops](){
      for(auto item : subscribed_markets_ops)
      {
         auto itr = _market_subscriptions.find(item.first);
         if(itr != _market_subscriptions.end())
            itr->second(fc::variant(item.second));
      }
   });
}


int database_api::is_asset_name_valid(const string asset_name)
{
  return my->is_asset_name_valid( asset_name );
}

/*
 * 创建通证时，判断输入的资产名称是否合法
 * return: 0: 合法 1：资产名称格式错误 2：资产名称为系统保留的资产名称 3：资产名称已存在 4:资产名称包含敏感词
*/ 
int database_api_impl::is_asset_name_valid(const string asset_name)
{
  database& d = _db;
  const module_cfg_object& m = d.get_module_cfg("TOKEN");
  const token_change_profie token_profile = get_token_profile(m.module_cfg);
  
  //asset_name
  unsigned int len = asset_name.size();
  //FC_ASSERT( len >= token_profile.name_length.min, "errno=10201003, asset_name is too short. asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));
  //FC_ASSERT( len <= token_profile.name_length.max, "errno=10201004, asset_name is too long. asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));
  if( len < token_profile.name_length.min )
      return 1;

  if( len > token_profile.name_length.max )
      return 1;      
  
  char c = 0;
  for(unsigned int i = 0; i < len; i++) //只允许大小写字母
  {  
      c = asset_name[i];
      //FC_ASSERT( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'), "errno=10201037, asset_name can only contain letters");
      if( !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) )
      {
          return 1;
      }
  }

  string upper_case_asset_name;
  std::transform(asset_name.begin(), asset_name.end(), std::back_inserter(upper_case_asset_name), (int (*)(int))toupper);

  //检查是否是系统保留的
  for (auto temp_itr = token_profile.reserved_asset_names.begin(); temp_itr != token_profile.reserved_asset_names.end(); ++temp_itr)
  {
    //FC_ASSERT(upper_case_asset_name != *temp_itr, "errno=10201053, asset_name: ${asset_name} is reserved by system", ("asset_name", op.template_parameter.asset_name));
    if( upper_case_asset_name == *temp_itr )
        return 2;
  }

  for (auto temp_itr = token_profile.reserved_asset_symbols.begin(); temp_itr != token_profile.reserved_asset_symbols.end(); ++temp_itr)
  {
    //FC_ASSERT(upper_case_asset_name != *temp_itr, "errno=10201053, asset_name: ${asset_name} is reserved by system", ("asset_name", op.template_parameter.asset_name));
    if( upper_case_asset_name == *temp_itr )
        return 2;
  }

  const auto& idx = d.get_index_type<token_index>().indices().get<by_upper_case_asset_name>();
  auto itr = idx.find(upper_case_asset_name);//如果这里为字符串null的话会导致程序崩溃  
  //FC_ASSERT( itr == idx.end(), "errno=10201038, asset_name already exists, asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));
  if( itr != idx.end())
      return 3;

  //check if asset_name has existed within asset_symbol
  auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
  auto asset_symbol_itr = asset_indx.find( upper_case_asset_name );
  //FC_ASSERT( asset_symbol_itr == asset_indx.end(), "errno=10201038, asset_name already exists, asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));
  if( asset_symbol_itr != asset_indx.end())
      return 3;

    //敏感词检查
    string lower_case_asset_name;
  std::transform(asset_name.begin(), asset_name.end(), std::back_inserter(lower_case_asset_name), (int (*)(int))tolower);
  string result = word_contain_sensitive_word(lower_case_asset_name, d);
  //FC_ASSERT( result == "", "errno=10201046, asset_name contains sensitive_word: ${result}", ("result", result));
  if( result != "")
      return 4;

  return 0; //合法
}


int database_api::is_asset_symbol_valid(const string asset_symbol)
{
  return my->is_asset_symbol_valid( asset_symbol );
}

/*
 * 创建通证时，判断输入的资产缩写是否合法
 * return: 0: 合法 1：资产缩写格式错误 2：资产缩写为系统保留的资产缩写 3：资产缩写已存在 4:资产缩写包含敏感词
*/ 
int database_api_impl::is_asset_symbol_valid(const string asset_symbol)
{
  database& d = _db;
  const module_cfg_object& m = d.get_module_cfg("TOKEN");
  const token_change_profie token_profile = get_token_profile(m.module_cfg);

  //asset_symbol
  unsigned int len = asset_symbol.size();
  //FC_ASSERT( len >= token_profile.symbol_length.min, "errno=10201005, asset_symbol is too short");
  //FC_ASSERT( len <= token_profile.symbol_length.max, "errno=10201006, asset_symbol is too long");
  if( len < token_profile.symbol_length.min )
      return 1;

  if( len > token_profile.symbol_length.max )
      return 1;   

  char c = 0; 
  for(unsigned int i = 0; i < len; i++)  
  {  
      c = asset_symbol[i];
      //FC_ASSERT( c >= 'A' && c <= 'Z', "errno=10201007, asset symbol can only contains characters: 'A'-'Z'");
      if( !(c >= 'A' && c <= 'Z') )
      {
          return 1;
      }     
  }

  //检查是否是系统保留的
  for (auto temp_itr = token_profile.reserved_asset_names.begin(); temp_itr != token_profile.reserved_asset_names.end(); ++temp_itr)
  {
    //FC_ASSERT(op.template_parameter.asset_symbol != *temp_itr, "errno=1020154, asset_symbol: ${asset_symbol} is reserved by system", ("asset_symbol", op.template_parameter.asset_symbol));
    if( asset_symbol == *temp_itr )
      return 2;
  }
  for (auto temp_itr = token_profile.reserved_asset_symbols.begin(); temp_itr != token_profile.reserved_asset_symbols.end(); ++temp_itr)
  {
    //FC_ASSERT(op.template_parameter.asset_symbol != *temp_itr, "errno=1020154, asset_symbol: ${asset_symbol} is reserved by system", ("asset_symbol", op.template_parameter.asset_symbol));
    if( asset_symbol == *temp_itr )
      return 2;
  }
    

  //查找用户资产id
  const auto& idx = d.get_index_type<token_index>().indices().get<by_upper_case_asset_name>();
  auto itr = idx.find(asset_symbol);//如果这里为字符串null的话会导致程序崩溃  
  //FC_ASSERT( itr == idx.end(), "errno=10201039, asset_symbol already exists, asset_symbol=${asset_symbol}", ("asset_symbol", op.template_parameter.asset_symbol));
  if( itr != idx.end() )
    return 3;

  //check if asset_symbol has existed within asset_name
  auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
  auto asset_symbol_itr = asset_indx.find( asset_symbol );
  //FC_ASSERT( asset_symbol_itr == asset_indx.end(), "errno=10201039, asset_symbol already exists, asset_symbol=${asset_symbol}", ("asset_symbol", op.template_parameter.asset_symbol));
  if( asset_symbol_itr != asset_indx.end() )
    return 3;

  //敏感词检查
  string lower_case_asset_symbol;
  std::transform(asset_symbol.begin(), asset_symbol.end(), std::back_inserter(lower_case_asset_symbol), (int (*)(int))tolower);
  string result = word_contain_sensitive_word(lower_case_asset_symbol, d);
  //FC_ASSERT( result == "", "errno=10201047, asset_symbol contains sensitive_word: ${result}", ("result", result));
  if( result != "" )
    return 4;

  return 0; //合法  
}


std::vector<token_brief> database_api_impl::get_tokens_brief_impl(const token_query_condition &condition, query_token_type type)const
{
	//打印传入参数
	elog("start [${start}] , limit[${limit}]", ("start", condition.start)("limit", condition.limit));
	elog("start time[${stime}] , end time[${etime}]", ("stime", condition.start_time)("etime", condition.end_time));
	elog("order_by[${order_by}],status[${status}],my_account[${my_account}] ", ("status", condition.status)("order_by", condition.order_by)("my_account", condition.my_account));

	//"create_time" | "end_time" |"buy_amount" | "buyer_number" | "guaranty_credit"
	if (condition.order_by == "create_time")
	{
		return get_tokens_brief_template_by_global_negative<by_create_time>(condition, type, "by_create_time");
	}
	else if (condition.order_by == "end_time")
	{
		return get_tokens_brief_template_by_global_positive<by_end_time>(condition, type, "by_end_time");
	}

	//认购AGC最多
 	else if (condition.order_by == "actual_core_asset_total")
 	{
 		return get_tokens_brief_template_by_statistics_negative<by_actual_core_asset_total>(condition, type);
 	}
	//认购人数最多
	else if (condition.order_by == "buyer_number")
	{
	 	return get_tokens_brief_template_by_statistics_negative<by_buyer_number>(condition, type);
	}
	//抵押信用最多
 	else if (condition.order_by == "guaranty_credit")
 	{
 		return get_tokens_brief_template_by_global_negative<by_guaranty_credit>(condition, type);
 	}
	else
	{
		return std::vector<token_brief>();
	}

}

std::vector<token_brief> database_api::get_tokens_brief(const token_query_condition &condition)const
{
	return my->get_tokens_brief( condition );
}

std::vector<token_brief> database_api_impl::get_tokens_brief(const token_query_condition &condition)const
{
	return get_tokens_brief_impl(condition, market_tokens_query);
}


std::vector<token_brief> database_api::my_get_tokens_brief(const token_query_condition &condition)const
{
	return my->my_get_tokens_brief(condition);
}

std::vector<token_brief> database_api_impl::my_get_tokens_brief(const token_query_condition &condition)const
{
	return get_tokens_brief_impl(condition, my_tokens_query);
}

optional<token_brief> database_api::get_token_brief_by_symbol_or_id(const string &token_symbol_or_id, const string &my_account)const
{
	return my->get_token_brief_by_symbol_or_id( token_symbol_or_id, my_account );
}

optional<token_brief> database_api_impl::get_token_brief_by_symbol_or_id(const string &token_symbol_or_id, const string &my_account)const
{
	return get_token_brief_by_symbol_or_id_impl(token_symbol_or_id, my_account, market_tokens_query);
}

optional<token_brief> database_api::my_get_token_brief_by_symbol_or_id(const string &token_symbol_or_id, const string &my_account)const
{
	return my->my_get_token_brief_by_symbol_or_id( token_symbol_or_id, my_account );
}
optional<token_brief> database_api_impl::my_get_token_brief_by_symbol_or_id(const string &token_symbol_or_id, const string &my_account)const
{
	return get_token_brief_by_symbol_or_id_impl(token_symbol_or_id, my_account, my_tokens_query);
}

optional<token_brief> database_api_impl::get_token_brief_by_symbol_or_id_impl(const string &token_symbol_or_id, const string &my_account, query_token_type type)const
{
	elog("token_symbol_or_id[${token_symbol_or_id}], my_account[${my_account}]", ("token_symbol_or_id", token_symbol_or_id)("my_account", my_account));

	object_id_type token_id = object_id_type(0,0,0);
	if ( token_symbol_or_id == "" || token_symbol_or_id == "null" )
	{
		elog("token name or id [${account}] is empty.", ("token",token_symbol_or_id) );
		return {};
	}
	if (std::isdigit(token_symbol_or_id[0]))
      {
            const token_object *to = _db.find(fc::variant(token_symbol_or_id).as<token_id_type>());
            if ( to == nullptr )
            {
                  return {};
            }
		token_id = to->id;
      }
	else
	{
		const token_object* token = nullptr;
		const auto& idx = _db.get_index_type<token_index>().indices().get<by_asset_symbol>();
		auto itr = idx.find(token_symbol_or_id);//如果这里为字符串null的话会导致程序崩溃
		if (itr != idx.end())
			token = &*itr;
		if (token != nullptr)
		{
			token_id = token->id;
		}
	}

      //没找到token
      if (token_id == object_id_type(0,0,0))
      {
            return {};
      }

	const auto& idx = _db.get_index_type<token_index>().indices().get<by_id>();
	auto itr = idx.find(token_id);
	if (itr != idx.end())
	{
		token_brief one;

		if (itr->control == token_object::token_control::unavailable)
		{
			return {};
		}
		else if (itr->control == token_object::token_control::description_forbidden)
		{
			one.brief       = "This token is description forbidden.";
      one.description = "This token is description forbidden.";
		}
		else
		{
			one.brief       = itr->template_parameter.brief;
      one.description = itr->template_parameter.description;
		}

		one.status             = itr->status;
		one.issuer             = _db.find(itr->issuer)->name;
		one.guaranty_credit    = itr->guaranty_credit;
		one.control            = itr->control;
		one.logo_url           = itr->template_parameter.logo_url;
		one.asset_name         = itr->template_parameter.asset_name;
		one.asset_symbol       = itr->template_parameter.asset_symbol;
		one.max_supply         = itr->template_parameter.max_supply;//token的最大供应量
		one.plan_buy_total     = itr->template_parameter.plan_buy_total;//要改成plan_buy_total
            one.buy_succeed_min_percent = itr->template_parameter.buy_succeed_min_percent;
		one.need_raising       = itr->template_parameter.need_raising;

		one.create_time  = itr->status_expires.create_time;
		one.phase1_begin = itr->status_expires.phase1_begin; // 认购第1阶段开始时间
		one.phase1_end   = itr->status_expires.phase1_end; // 认购第1阶段结束时间
		one.phase2_begin = itr->status_expires.phase2_begin; // 认购第2阶段开始时间
		one.phase2_end   = itr->status_expires.phase2_end;
		one.settle_time  = itr->status_expires.settle_time;//token的结算时间


		one.guaranty_core_asset_amount = itr->template_parameter.guaranty_core_asset_amount;//抵押AGC数量



		//根据itr->statistics查找众筹动态信息
		token_statistics_object* token_statistics = (token_statistics_object*)_db.find_object(itr->statistics);
		if (token_statistics == NULL)//还没开始众筹
		{
			elog("token statistics id ${id} is not found.", ("id", itr->statistics));
		}
		else
		{
			//填写动态统计信息
			one.token_id                   = token_statistics->token_id;
			one.actual_buy_amount          = token_statistics->actual_buy_total;//所有参与的用户已经认购的用户资产数量，这个考虑一下怎么算
			one.actual_core_asset_total    = token_statistics->actual_core_asset_total;//所有参与的用户已经募集的AGC数量
			one.buyer_number               = token_statistics->buyer_number;
		}

		one.buy_count = 0;
		//查看当前账户是否参与过这个众筹项目
		if ( my_account == "" || my_account == "null" )
		{
			elog("my account name or id [${account}] is empty.", ("account",my_account) );
		}
		else
		{
			const account_object* account = nullptr;
			if (std::isdigit(my_account[0]))
				account = _db.find(fc::variant(my_account).as<account_id_type>());
			else
			{
				const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
				auto itr = idx.find(my_account);//如果这里为字符串null的话会导致程序崩溃
				if (itr != idx.end())
					account = &*itr;
			}
			if (account != nullptr)
			{
				const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_id>();
				auto itr_buy = idx_buy.begin();

				for(;itr_buy != idx_buy.end(); itr_buy++)
				{
					if(itr_buy->buyer == account->id && itr_buy->token_id == one.token_id)
					{
						one.buy_count++;
						if (type == my_tokens_query)
						{
							one.my_participate.push_back(itr_buy->template_parameter);
						}						
					}
				}
			}
		}
            if( itr->exts )
                  one.extend_field = *(itr->exts);
		return one;
	}
	else
		return {};
}

std::vector<token_brief> database_api::my_get_my_create_tokens_brief(const token_query_condition &condition)const
{
	return my->my_get_my_create_tokens_brief( condition );
}
std::vector<token_brief> database_api_impl::my_get_my_create_tokens_brief(const token_query_condition &condition)const
{
	uint32_t count = 0;
	uint32_t index = 1;
	uint32_t num = condition.limit <= MAX_TOKEN_NUM_FOR_QUERY_RESULTS ? condition.limit : MAX_TOKEN_NUM_FOR_QUERY_RESULTS;
	std::vector<token_brief> result; result.reserve(num);

	account_id_type my_id;
	if ( condition.my_account == "" || condition.my_account == "null" )
	{
		elog("my account name or id [${my_account}] is empty.", ("my_account",condition.my_account) );
	}
	else
	{
		const account_object* account = nullptr;
		if (std::isdigit(condition.my_account[0]))
			account = _db.find(fc::variant(condition.my_account).as<account_id_type>());
		else
		{
			const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
			auto itr = idx.find(condition.my_account);//如果这里为字符串null的话会导致程序崩溃
			if (itr != idx.end())
				account = &*itr;
		}
		if (account != nullptr)
		{
			my_id = account->id;
		}
		else
			return result;
	}

	const auto& idx = _db.get_index_type<token_index>().indices().get<by_id>();
	auto itr = idx.begin();
	for(;itr != idx.end(); itr++, index++)
	{
		if(index >= condition.start)
		{
			if (itr->issuer == my_id)
			{
				token_brief one;

				if (itr->control == token_object::token_control::unavailable)
				{
					continue;
				}
				else if (itr->control == token_object::token_control::description_forbidden)
				{
					one.brief       = "This token is description forbidden.";
          one.description = "This token is description forbidden.";
				}
				else
				{
					one.brief       = itr->template_parameter.brief;
          one.description = itr->template_parameter.description;
				}

				one.status                  = itr->status;
				one.issuer                  = _db.find(itr->issuer)->name;
				one.guaranty_credit         = itr->guaranty_credit;
				one.control                 = itr->control;
				one.logo_url                = itr->template_parameter.logo_url;
				one.asset_name              = itr->template_parameter.asset_name;
				one.asset_symbol            = itr->template_parameter.asset_symbol;
				one.max_supply              = itr->template_parameter.max_supply;//token的最大供应量
				one.plan_buy_total          = itr->template_parameter.plan_buy_total;
                        one.buy_succeed_min_percent = itr->template_parameter.buy_succeed_min_percent;
				one.need_raising            = itr->template_parameter.need_raising;

				one.create_time  = itr->status_expires.create_time;
				one.phase1_begin = itr->status_expires.phase1_begin; // 认购第1阶段开始时间
				one.phase1_end   = itr->status_expires.phase1_end; // 认购第1阶段结束时间
				one.phase2_begin = itr->status_expires.phase2_begin; // 认购第2阶段开始时间
				one.phase2_end   = itr->status_expires.phase2_end;
				one.settle_time  = itr->status_expires.settle_time;//token的结算时间


				one.guaranty_core_asset_amount = itr->template_parameter.guaranty_core_asset_amount;//抵押AGC数量
				one.buy_count                  = 1;


				//根据itr->statistics查找投票统计的动态信息
				token_statistics_object* token_statistics = (token_statistics_object*)_db.find_object(itr->statistics);
				if (token_statistics == NULL)//主题创建了，但是还没有投票
				{
					elog("subject statistics id ${id} is not found.", ("id", itr->statistics));
				}
				else
				{
					one.token_id                   = token_statistics->token_id;
					one.actual_buy_amount          = token_statistics->actual_buy_total;//所有参与的用户已经认购的用户资产数量，这个考虑一下怎么算
					one.actual_core_asset_total    = token_statistics->actual_core_asset_total;//所有参与的用户已经募集的AGC数量
					one.buyer_number               = token_statistics->buyer_number;
				}

				TokenFillExtendField(itr->exts, condition.extra_query_fields, one.extend_field);
				result.push_back(one);
				++count;

				if (count >= num)
					break;
			}
		}  
	}
	return result;
}


void database_api_impl::TokenFillExtendField(const optional<map<string, string> >  &token_exts,const set<string> &ext_field,map<string, string> &ret_ext_field)const 
{
	if( token_exts ) 
	{
		if(ext_field.empty())
		{
			ret_ext_field = *token_exts;
		}
		for(auto x:ext_field)
		{
			//查询token_ext中是否有这个字段
			if (token_exts->find(x) != token_exts->end())
			{
				ret_ext_field.insert(make_pair(x, token_exts->at(x)));
			}
		}
	}
	return;
}

optional<token_detail> database_api::get_token_detail(const string &token_symbol_or_id, const string &my_account)const
{
	return my->get_token_detail( token_symbol_or_id, my_account );
}

optional<token_detail> database_api_impl::get_token_detail(const string &token_symbol_or_id, const string &my_account)const
{
	elog("token_symbol_or_id[${token_symbol_or_id}], my_account[${my_account}]", ("token_symbol_or_id", token_symbol_or_id)("my_account", my_account));

	object_id_type token_id(0,0,0);
	if ( token_symbol_or_id == "" || token_symbol_or_id == "null" )
	{
		elog("token name or id [${account}] is empty.", ("token",token_symbol_or_id) );
		return {};
	}
	if (std::isdigit(token_symbol_or_id[0]))
      {
            const token_object *to = _db.find(fc::variant(token_symbol_or_id).as<token_id_type>());
            if ( to == nullptr )
            {
                  return {};
            }
            token_id = to->id;
      }
	else
	{
		const token_object* token = nullptr;
		const auto& idx = _db.get_index_type<token_index>().indices().get<by_asset_symbol>();
		auto itr = idx.find(token_symbol_or_id);//如果这里为字符串null的话会导致程序崩溃
		if (itr != idx.end())
			token = &*itr;
		if (token != nullptr)
		{
			token_id = token->id;
		}
	}

      //没找到token
      if ( token_id == object_id_type(0,0,0))
      {
            return {};
      }

	const auto& idx = _db.get_index_type<token_index>().indices().get<by_id>();
	auto itr = idx.find(token_id);
	if (itr != idx.end())
	{
		token_detail one;

		if (itr->control == token_object::token_control::unavailable)
		{
			return {};
		}
		else if (itr->control == token_object::token_control::description_forbidden)
		{
			one.brief       = "This token is description forbidden.";
			one.description = "This token is description forbidden.";
		}
		else
		{
			one.brief       = itr->template_parameter.brief;
			one.description = itr->template_parameter.description;
		}

		one.control                     = itr->control;
		one.issuer                      = _db.find(itr->issuer)->name;
		one.status                      = itr->status;
		one.guaranty_credit             = itr->guaranty_credit;
		one.asset_name                  = itr->template_parameter.asset_name;
		one.asset_symbol                = itr->template_parameter.asset_symbol;
		one.user_issued_asset_id        = itr->user_issued_asset_id;
		one.type                        = itr->template_parameter.type;
		one.subtype                     = itr->template_parameter.subtype;
		one.logo_url                    = itr->template_parameter.logo_url;
		one.max_supply                  = itr->template_parameter.max_supply;
		one.plan_buy_total              = itr->template_parameter.plan_buy_total;
		one.buy_succeed_min_percent     = itr->template_parameter.buy_succeed_min_percent;
		one.not_buy_asset_handle        = itr->template_parameter.not_buy_asset_handle;
		one.guaranty_core_asset_amount  = itr->template_parameter.guaranty_core_asset_amount;
		one.guaranty_core_asset_months  = itr->template_parameter.guaranty_core_asset_months;
		one.need_raising                = itr->template_parameter.need_raising;
		one.create_time                 = itr->status_expires.create_time;
		one.phase1_end                  = itr->status_expires.phase1_end;
		one.settle_time                 = itr->status_expires.settle_time;
		one.result                      = itr->result;
		one.buy_phases                  = itr->template_parameter.buy_phases;
		one.whitelist                   = itr->template_parameter.whitelist;
		one.customized_attributes       = itr->template_parameter.customized_attributes;
		one.issuer_reserved_asset_frozen_months = itr->template_parameter.issuer_reserved_asset_frozen_months;

		//TokenFillExtendField(itr->exts, condition.extra_query_fields, one.extend_field);
            if ( itr->exts )
		    one.extend_field = *(itr->exts);
		


		//根据itr->statistics查找众筹动态信息
		token_statistics_object* token_statistics = (token_statistics_object*)_db.find_object(itr->statistics);
		if (token_statistics == NULL)//还没开始众筹
		{
			elog("token statistics id ${id} is not found.", ("id", itr->statistics));
		}
		else
		{
			//填写动态统计信息
			one.actual_buy_percentage      = token_statistics->actual_buy_percentage;
			one.actual_not_buy_total       = token_statistics->actual_not_buy_total;
			one.token_id                   = token_statistics->token_id;
			one.actual_buy_total           = token_statistics->actual_buy_total;//所有参与的用户已经认购的用户资产数量，这个考虑一下怎么算
			one.actual_core_asset_total    = token_statistics->actual_core_asset_total;//所有参与的用户已经募集的AGC数量
			one.buyer_number               = token_statistics->buyer_number;
			one.return_issuer_reserved_asset_detail = token_statistics->return_issuer_reserved_asset_detail;
			one.return_guaranty_core_asset_detail   = token_statistics->return_guaranty_core_asset_detail;
		}

		one.buy_count = 0;
		//查看当前账户是否参与过这个众筹项目
		if ( my_account == "" || my_account == "null" )
		{
			elog("my account name or id [${account}] is empty.", ("account",my_account) );
		}
		else
		{
			const account_object* account = nullptr;
			if (std::isdigit(my_account[0]))
				account = _db.find(fc::variant(my_account).as<account_id_type>());
			else
			{
				const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
				auto itr = idx.find(my_account);//如果这里为字符串null的话会导致程序崩溃
				if (itr != idx.end())
					account = &*itr;
			}
			if (account != nullptr)
			{
				const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_id>();
				auto itr_buy = idx_buy.begin();

				for(;itr_buy != idx_buy.end(); itr_buy++)
				{
					if(itr_buy->buyer == account->id && itr_buy->token_id == one.token_id)
					{
						one.buy_count++;
						one.my_participate.push_back(itr_buy->template_parameter);
					}
				}
			}
		}

		return one;
	}
	else
		return {};
}

vector<token_buy_object> database_api::get_buy_token_detail(object_id_type token_id, const string &my_account)const
{
	return my->get_buy_token_detail( token_id, my_account );
}

vector<token_buy_object> database_api_impl::get_buy_token_detail(object_id_type token_id, const string &my_account)const
{
	elog("token_id[${token_id}], my_account[${my_account}]", ("token_id", token_id)("my_account", my_account));
	vector<token_buy_object> result;
	const auto& idx = _db.get_index_type<token_index>().indices().get<by_id>();
	auto itr = idx.find(token_id);
	if (itr != idx.end())
	{
		//查看当前账户是否参与过这个众筹项目
		if ( my_account == "" || my_account == "null" )
		{
			elog("my account name or id [${account}] is empty.", ("account",my_account) );
		}
		else
		{
			const account_object* account = nullptr;
			if (std::isdigit(my_account[0]))
				account = _db.find(fc::variant(my_account).as<account_id_type>());
			else
			{
				const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
				auto itr = idx.find(my_account);//如果这里为字符串null的话会导致程序崩溃
				if (itr != idx.end())
					account = &*itr;
			}
			if (account != nullptr)
			{
				const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_id>();
				auto itr_buy = idx_buy.begin();

				for(;itr_buy != idx_buy.end(); itr_buy++)
				{
					if(itr_buy->buyer == account->id && itr_buy->token_id == token_id)
					{
						result.push_back(*itr_buy);
					}
				}
			}
		}
	}
	return result;
}

uint32_t database_api::get_buy_record_total(object_id_type token_id, const string &issue_account)const
{
	return my->get_buy_record_total( token_id, issue_account );
}
uint32_t database_api_impl::get_buy_record_total(object_id_type token_id, const string &issue_account)const
{
	elog("token_id[${token_id}], issue_account[${issue_account}]", ("token_id", token_id)("issue_account", issue_account));
	
	const auto& idx = _db.get_index_type<token_index>().indices().get<by_id>();
	auto itr = idx.find(token_id);
	if (itr != idx.end())
	{
		if ( issue_account == "" || issue_account == "null" )
		{
			elog("publish account name or id [${account}] is empty.", ("account",issue_account) );
		}
		else
		{
			const account_object* account = nullptr;
			if (std::isdigit(issue_account[0]))
				account = _db.find(fc::variant(issue_account).as<account_id_type>());
			else
			{
				const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
				auto itr = idx.find(issue_account);//如果这里为字符串null的话会导致程序崩溃
				if (itr != idx.end())
					account = &*itr;
			}
			if (account != nullptr)
			{
				//验证issue_account的合法性
				if(itr->issuer == account->id)
				{
					//开始统计
					const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_token_id>();
                    auto itr_buy_begin = idx_buy.lower_bound(token_id);
                    auto itr_buy_end = idx_buy.upper_bound(token_id);
                    uint32_t count = distance(itr_buy_begin, itr_buy_end);
                    return count;
				}
				else
				{
					//抛出异常
					FC_ASSERT(itr->issuer != account->id, "wrong issue account.currect issue account id is ${issuer}", ("issuer", itr->issuer) );
					return 0;
				}
			}
		}
	}
	FC_ASSERT(1, "token[${token_id}] not found", ("token_id", token_id) );
	return 0;
}


vector<token_buy_object> database_api::get_buy_list(uint32_t start, uint32_t limit, object_id_type token_id, const string &issue_account)const
{
	return my->get_buy_list( start, limit, token_id, issue_account );
}

vector<token_buy_object> database_api_impl::get_buy_list(uint32_t start, uint32_t limit, object_id_type token_id, const string &issue_account)const
{
	elog("start[${start}],limit[${limit}],token_id[${token_id}], issue_account[${issue_account}]", ("start", start)("limit", limit)("token_id", token_id)("issue_account", issue_account));

	uint32_t num = limit <= MAX_BUY_TOKEN_RECORD_NUM_FOR_QUERY_RESULTS ? limit : MAX_BUY_TOKEN_RECORD_NUM_FOR_QUERY_RESULTS;
	vector<token_buy_object> result;

	const auto& idx = _db.get_index_type<token_index>().indices().get<by_id>();
	auto itr = idx.find(token_id);
	if (itr != idx.end())
	{
        if ( !(itr->result.is_succeed) )//通证众筹失败
            return result;

		if ( issue_account == "" || issue_account == "null" )
		{
			elog("publish account name or id [${account}] is empty.", ("account",issue_account) );
		}
		else
		{
			const account_object* account = nullptr;
			if (std::isdigit(issue_account[0]))
				account = _db.find(fc::variant(issue_account).as<account_id_type>());
			else
			{
				const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
				auto itr = idx.find(issue_account);//如果这里为字符串null的话会导致程序崩溃
				if (itr != idx.end())
					account = &*itr;
			}
			if (account != nullptr)
			{
				//验证issue_account的合法性
				if(itr->issuer == account->id)
				{
					//查看查询次数是否超了限制值
					static map<object_id_type, uint32_t> get_records_num_sum;
					map<object_id_type, uint32_t>::iterator it_find = get_records_num_sum.find(token_id);
					if( it_find==get_records_num_sum.end() )
					{
						get_records_num_sum.insert(make_pair(token_id,0));
						elog("This is the first time get_buy_list[${token_id}]", ("token_id", token_id));
					}
					else
					{
						elog("You have get_buy_list for record num [${num}],max num[${max_num}]", ("num", get_records_num_sum[token_id])("max_num", MAX_GET_BUY_LIST_RECORD_NUM));
						if ( get_records_num_sum[token_id] >= MAX_GET_BUY_LIST_RECORD_NUM )
						{
							FC_ASSERT(get_records_num_sum[token_id] >= MAX_GET_BUY_LIST_RECORD_NUM, "You have get_buy_list for record num [${num}],max num[${max_num}]", ("num", get_records_num_sum[token_id])("max_num", MAX_GET_BUY_LIST_RECORD_NUM) );
							return {};
						}
					}
					


					//开始统计
					const auto& idx_buy = _db.get_index_type<token_buy_index>().indices().get<by_token_id>();
                    auto itr_buy_begin = idx_buy.lower_bound(token_id);
                    auto itr_buy_end = idx_buy.upper_bound(token_id);
                    std::advance(itr_buy_begin,start);
					uint32_t count = 0;
                    for(auto itr_buy = itr_buy_begin;itr_buy != itr_buy_end && count<num; itr_buy++, count++)
					{
						result.push_back(*itr_buy);
					}
					get_records_num_sum[token_id] += result.size();
				}
				else
				{
					//抛出异常
					FC_ASSERT(itr->issuer != account->id, "wrong issue account.currect issue account id is ${issuer}", ("issuer", itr->issuer) );
				}
			}
		}
	}
	return result;
}

//[end]

std::vector<token_object> database_api::get_tokens_by_collected_core_asset( uint32_t start, uint32_t limit )const
{
    return my->get_tokens_by_collected_core_asset( start, limit );
}

std::vector<token_object> database_api_impl::get_tokens_by_collected_core_asset(uint32_t start, uint32_t limit)const
{
   uint32_t count = 0;
   uint32_t index = 1;
   uint32_t num = limit <= MAX_TOKEN_NUM_FOR_QUERY_RESULTS ? limit : MAX_TOKEN_NUM_FOR_QUERY_RESULTS;
   std::vector<token_object> results; results.reserve(num);

   return results;
}

optional<token_object> database_api::get_token_by_id(const token_id_type token_id )const
{
    return my->get_token_by_id( token_id );
}

optional<token_object> database_api_impl::get_token_by_id(const token_id_type token_id)const
{
    return optional<token_object>();
}

optional<token_object> database_api::get_token_by_asset_name( string asset_name )const
{
    return my->get_token_by_asset_name( asset_name );
}

// 不区分通证名称大小写
optional<token_object> database_api_impl::get_token_by_asset_name(const string asset_name)const
{
    string upper_case_asset_name;
    std::transform(asset_name.begin(), asset_name.end(), std::back_inserter(upper_case_asset_name), (int (*)(int))toupper);

    const auto& idx = _db.get_index_type<token_index>().indices().get<by_upper_case_asset_name>();

    auto itr = idx.find(upper_case_asset_name);

    if (itr != idx.end())
    {
      return *itr;
    }
    return {};
}

size_t database_api::get_token_total() const
{
    return my->get_token_total();
}

size_t database_api_impl::get_token_total() const
{
    const auto& idx = _db.get_index_type<token_index>().indices().get<by_upper_case_asset_name>();
    return idx.size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// delay transfer                                                   //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::vector<delay_transfer_object_for_query> database_api::get_delay_transfer_by_from( uint32_t start, uint32_t limit, string from_name_or_id, uint8_t query_type )const
{
   return my->get_delay_transfer_by_from( start, limit, from_name_or_id, query_type );
}

/**
 *  query_type: 0 所有延迟转账 1 没执行的延迟转账 2 已执行的延迟转账
 *  start >= 1
 */
std::vector<delay_transfer_object_for_query> database_api_impl::get_delay_transfer_by_from( uint32_t start, uint32_t limit, string from_name_or_id, uint8_t query_type )const
{
      uint32_t count = 0;
      uint32_t index = 1;
      uint32_t num = limit <= MAX_DELAY_TRANSFER_NUM_FOR_QUERY_RESULTS ? limit : MAX_DELAY_TRANSFER_NUM_FOR_QUERY_RESULTS;
      std::vector<delay_transfer_object_for_query> results; results.reserve(num);

    if ( from_name_or_id == "" || from_name_or_id == "null" )
    {
        elog("from_name_or_id [${account}] is empty.", ("account",from_name_or_id) );
    }
    else
    {
        const account_object* from_account = nullptr;
        const account_object* to_account = nullptr;

        if (std::isdigit(from_name_or_id[0]))
            from_account = _db.find(fc::variant(from_name_or_id).as<account_id_type>());
        else
        {
            const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
            auto itr = idx.find(from_name_or_id);//如果这里为字符串null的话会导致程序崩溃
            if (itr != idx.end())
                from_account = &*itr;
        }

        if (from_account != nullptr)
        {
            const auto& idx = _db.get_index_type<delay_transfer_index>().indices().get<by_from>();

            if( idx.begin() != idx.end() )
            {
                //处理每个转账接收人
                auto itr = idx.end();
                do
                {
                    --itr;
                    to_account = &(itr->to(_db));
                    //ilog("get_delay_transfer_by_from from=${a}, to=${b}", ("a", from_name_or_id)("b", to_account->name));

                    if( itr->from == from_account->id && 
                        ( query_type == 0 || (query_type == 1 && !itr->finished) || (query_type == 2 && itr->finished) ) )
                    {
                        if (index >= start )
                        {
                            //ilog("get_delay_transfer_by_from from=${a}, to=${b}, index=${i}", ("a", from_name_or_id)("b", to_account->name)("i", index));

                            asset_object a = itr->delay_transfer_detail[0].info.transfer_asset.asset_id(_db);

                            delay_transfer_object_for_query temp;
                             
                            temp.id                      = itr->id;
                            temp.from                    = itr->from;
                            temp.to                      = itr->to;
                            temp.operation_time          = itr->operation_time;
                            temp.delay_transfer_detail   = itr->delay_transfer_detail;
                            temp.finished                = itr->finished;
                            temp.exts                    = itr->exts;

                            temp.from_name               = from_account->name;
                            temp.to_name                 = to_account->name;
                            temp.transfer_asset_symbol   = a.symbol;

                            results.push_back(temp);
                            ++count;

                            if (count >= num)
                                break;
                        }

                        ++index;
                    }
                }
                while(itr != idx.begin());
            }
         } //if (from_account != nullptr)
   }

   return results;
}


std::vector<delay_transfer_object_for_query> database_api::get_delay_transfer_by_to( uint32_t start, uint32_t limit, string to_name_or_id, uint8_t query_type )const
{
   return my->get_delay_transfer_by_to( start, limit, to_name_or_id, query_type );
}

/**
 *  query_type: 0 所有延迟转账 1 没执行的延迟转账 2 已执行的延迟转账
 *  start >= 1
 */
std::vector<delay_transfer_object_for_query> database_api_impl::get_delay_transfer_by_to( uint32_t start, uint32_t limit, string to_name_or_id, uint8_t query_type )const
{
      uint32_t count = 0;
      uint32_t index = 1;
      uint32_t num = limit <= MAX_DELAY_TRANSFER_NUM_FOR_QUERY_RESULTS ? limit : MAX_DELAY_TRANSFER_NUM_FOR_QUERY_RESULTS;
      std::vector<delay_transfer_object_for_query> results; results.reserve(num);

    if ( to_name_or_id == "" || to_name_or_id == "null" )
    {
        elog("to_name_or_id [${account}] is empty.", ("account",to_name_or_id) );
    }
    else
    {
        const account_object* from_account = nullptr;
        const account_object* to_account = nullptr;

        if (std::isdigit(to_name_or_id[0]))
            to_account = _db.find(fc::variant(to_name_or_id).as<account_id_type>());
        else
        {
            const auto& i = _db.get_index_type<account_index>().indices().get<by_name>();
            auto iter = i.find(to_name_or_id);//如果这里为字符串null的话会导致程序崩溃
            if (iter != i.end())
                to_account = &*iter;
        }
        if (to_account != nullptr )
        {
            const auto& idx = _db.get_index_type<delay_transfer_index>().indices().get<by_to>();
            if( idx.begin() != idx.end() )
            {
                auto itr = idx.end();
                do 
                {  
                    --itr;
                    from_account = &(itr->from(_db));
                    //ilog("get_delay_transfer_by_from from=${f}, to=${t}", ("f", from_account->name)("t", to_account->name));

                    if( itr->to == to_account->id && 
                        ( query_type == 0 || (query_type == 1 && !itr->finished) || (query_type == 2 && itr->finished) ) )
                    {
                        if( index >= start )
                        {
                            asset_object a = itr->delay_transfer_detail[0].info.transfer_asset.asset_id(_db);

                            delay_transfer_object_for_query temp;
                             
                            temp.id                      = itr->id;
                            temp.from                    = itr->from;
                            temp.to                      = itr->to;
                            temp.operation_time          = itr->operation_time;
                            temp.delay_transfer_detail   = itr->delay_transfer_detail;
                            temp.finished                = itr->finished;
                            temp.exts                    = itr->exts;

                            temp.from_name               = from_account->name;
                            temp.to_name                 = to_account->name;
                            temp.transfer_asset_symbol   = a.symbol;

                            results.push_back(temp);
                            ++count;

                            if (count >= num)
                                break;
                        }

                        ++index;     
                    }
                }
                while(itr != idx.begin());
            }
        } //if (from_account != nullptr)
   }

   return results;
}


optional<delay_transfer_unexecuted_object> database_api::get_delay_transfer_unexecuted_asset_by_to( string to_name_or_id )const
{
   return my->get_delay_transfer_unexecuted_asset_by_to( to_name_or_id );
}

/**
 *  获取指定账号的每种没执行(待解冻)的资产的明细统计
 */
optional<delay_transfer_unexecuted_object> database_api_impl::get_delay_transfer_unexecuted_asset_by_to( string to_name_or_id )const
{
    if ( to_name_or_id == "" || to_name_or_id == "null" )
    {
        elog("to_name_or_id [${account}] is empty.", ("account",to_name_or_id) );
    }
    else
    {
        const account_object* to_account = nullptr;

        if (std::isdigit(to_name_or_id[0]))
            to_account = _db.find(fc::variant(to_name_or_id).as<account_id_type>());
        else
        {
            const auto& i = _db.get_index_type<account_index>().indices().get<by_name>();
            auto iter = i.find(to_name_or_id);//如果这里为字符串null的话会导致程序崩溃
            if (iter != i.end())
                to_account = &*iter;
        }
        if (to_account != nullptr)
        {
            const auto& unexecuted_object_index = _db.get_index_type<delay_transfer_unexecuted_index>().indices().get<by_to>();
            auto itr = unexecuted_object_index.find(to_account->id);

            if( itr != unexecuted_object_index.end() )
            {
                return *itr;
            }
        }
    }

    return {};
}

} } // graphene::app
