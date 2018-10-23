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


#include <graphene/chain/database.hpp>
#include <graphene/chain/token_object.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <stdio.h>
namespace graphene { namespace chain {

class token_fsm;
typedef enum token_object::token_status fsm_token_status;
typedef string fsm_token_event;

struct token_fsm_message
{
	int 	status;
	string	msg;
};

struct token_fsm_fun
{
	typedef bool (*fsm_cond)(token_fsm&, database& db);
    typedef bool (*fsm_action)(token_fsm&, database& db);
    typedef bool (*fsm_exec)(token_fsm&, database& db);
};

class fsm_token_create //: public fsm
{
public:
	static bool fsm_cond_create(token_fsm& fsm, database& db);
	static bool fsm_action_create(token_fsm& fsm, database& db);
	static bool fsm_exec_create(token_fsm& fsm, database& db);
};

class fsm_token_phase1_begin //: public fsm
{
public:
	static bool fsm_cond_phase1_begin(token_fsm& fsm, database& db);
	static bool fsm_action_phase1_begin(token_fsm& fsm, database& db);
	static bool fsm_exec_phase1_begin(token_fsm& fsm, database& db);
};

class fsm_token_phase1_end
{
public:
	static bool fsm_cond_phase1_end(token_fsm& fsm, database& db);
	static bool fsm_action_phase1_end(token_fsm& fsm, database& db);
	static bool fsm_exec_phase1_end(token_fsm& fsm, database& db);
};

class fsm_token_phase2_begin //: public fsm
{
public:
	static bool fsm_cond_phase2_begin(token_fsm& fsm, database& db);
	static bool fsm_action_phase2_begin(token_fsm& fsm, database& db);
	static bool fsm_exec_phase2_begin(token_fsm& fsm, database& db);
};

class fsm_token_phase2_end
{
public:
	static bool fsm_cond_phase2_end(token_fsm& fsm, database& db);
	static bool fsm_action_phase2_end(token_fsm& fsm, database& db);
	static bool fsm_exec_phase2_end(token_fsm& fsm, database& db);
};

class fsm_token_settle
{
public:
	static bool fsm_cond_settle(token_fsm& fsm, database& db);
	static bool fsm_action_settle(token_fsm& fsm, database& db);
	static bool fsm_exec_settle(token_fsm& fsm, database& db);
};

class fsm_token_return_asset_end
{
public:
	static bool fsm_cond_return_asset_end(token_fsm& fsm, database& db);
	static bool fsm_action_return_asset_end(token_fsm& fsm, database& db);
	static bool fsm_exec_return_asset_end(token_fsm& fsm, database& db);
};

class fsm_token_close //: public fsm
{
public:
	static bool fsm_cond_close(token_fsm& fsm, database& db);
	static bool fsm_action_close(token_fsm& fsm, database& db);
	static bool fsm_exec_close(token_fsm& fsm, database& db);
};

class fsm_token_restore //: public fsm
{
public:
	static bool fsm_cond_restore(token_fsm& fsm, database& db);
	static bool fsm_action_restore(token_fsm& fsm, database& db);
	static bool fsm_exec_restore(token_fsm& fsm, database& db);
};

class fsm_token_set_control //: public fsm
{
public:
	static bool fsm_cond_set_control(token_fsm& fsm, database& db);
	static bool fsm_action_set_control(token_fsm& fsm, database& db);
	static bool fsm_exec_set_control(token_fsm& fsm, database& db);
};

token_object get_token(database& db, const token_id_type& token_id)
{
	auto& token = db.get_index_type<token_index>().indices().get<by_id>();
	auto token_itr = token.find(token_id);
	FC_ASSERT( token_itr != token.end() );
	return *token_itr;
}

class token_fsm
{
public:

	struct fsm_entry
	{
		fsm_token_status 				current_state;
		fsm_token_event					event;
		fsm_token_status 				next_state;
		token_fsm_fun::fsm_cond 		cond;
		token_fsm_fun::fsm_action 		action;
		token_fsm_fun::fsm_exec 		exec;
	};

	token_fsm(database& db, const token_event_object& object)
		:e_object(object)
	{
		current_state_ = get_token(db, e_object.token_id).status;
		event_ 	= object.event;
	}

	int fsm_do(database& db, const std::function<void(token_fsm_message&)>& constructor);
	int fsm_init();
	int fsm_term();

	inline fsm_token_status get_current()	{ return current_state_; }
	inline fsm_token_event& get_event()	{ return event_; }
	inline time_point_sec 	  get_time()	{ return e_object.time;	}
	inline token_id_type get_token_id()	{ return e_object.token_id; }
	string event_test(const string& key) {
		return e_object.event_test(key);
	}

	string get_event_content() {
		return e_object.event_test("content");
	}

private:
	token_fsm() {}
	static std::vector<struct fsm_entry> fsm_entries_;
	fsm_token_status 		current_state_;
	fsm_token_event 		event_;
	fsm_token_status 		next_state_;
	token_event_object 	e_object;
};

// event： create|phase1_begin|phase1_end|phase2_begin|phase2_end|settle|return_asset_end|close|restore|set_control
std::vector<struct token_fsm::fsm_entry> token_fsm::fsm_entries_ = {
	token_fsm::fsm_entry{fsm_token_status::none_status, 		     "create", 			fsm_token_status::create_status, 			fsm_token_create::fsm_cond_create, fsm_token_create::fsm_action_create, fsm_token_create::fsm_exec_create},
	token_fsm::fsm_entry{fsm_token_status::create_status, 		 	 "phase1_begin",	fsm_token_status::phase1_begin_status,	 	fsm_token_phase1_begin::fsm_cond_phase1_begin, fsm_token_phase1_begin::fsm_action_phase1_begin, fsm_token_phase1_begin::fsm_exec_phase1_begin},
	token_fsm::fsm_entry{fsm_token_status::phase1_begin_status, 	 "phase1_end", 		fsm_token_status::phase1_end_status, 		fsm_token_phase1_end::fsm_cond_phase1_end, fsm_token_phase1_end::fsm_action_phase1_end, fsm_token_phase1_end::fsm_exec_phase1_end},
	token_fsm::fsm_entry{fsm_token_status::phase1_end_status,		 "phase2_begin",	fsm_token_status::phase2_begin_status, 		fsm_token_phase2_begin::fsm_cond_phase2_begin, fsm_token_phase2_begin::fsm_action_phase2_begin, fsm_token_phase2_begin::fsm_exec_phase2_begin},
	token_fsm::fsm_entry{fsm_token_status::phase2_begin_status, 	 "phase2_end", 		fsm_token_status::phase2_end_status,		fsm_token_phase2_end::fsm_cond_phase2_end, fsm_token_phase2_end::fsm_action_phase2_end, fsm_token_phase2_end::fsm_exec_phase2_end},
	token_fsm::fsm_entry{fsm_token_status::phase1_begin_status,		 "settle", 			fsm_token_status::settle_status,			fsm_token_settle::fsm_cond_settle, fsm_token_settle::fsm_action_settle, fsm_token_settle::fsm_exec_settle},
	token_fsm::fsm_entry{fsm_token_status::phase1_end_status,		 "settle", 			fsm_token_status::settle_status,			fsm_token_settle::fsm_cond_settle, fsm_token_settle::fsm_action_settle, fsm_token_settle::fsm_exec_settle},
	token_fsm::fsm_entry{fsm_token_status::phase2_begin_status,		 "settle", 			fsm_token_status::settle_status,			fsm_token_settle::fsm_cond_settle, fsm_token_settle::fsm_action_settle, fsm_token_settle::fsm_exec_settle},
	token_fsm::fsm_entry{fsm_token_status::phase2_end_status,		 "settle", 			fsm_token_status::settle_status,			fsm_token_settle::fsm_cond_settle, fsm_token_settle::fsm_action_settle, fsm_token_settle::fsm_exec_settle},
	token_fsm::fsm_entry{fsm_token_status::settle_status,			 "return_asset_end",fsm_token_status::return_asset_end_status,	fsm_token_return_asset_end::fsm_cond_return_asset_end, fsm_token_return_asset_end::fsm_action_return_asset_end, 
			fsm_token_return_asset_end::fsm_exec_return_asset_end},
	token_fsm::fsm_entry{fsm_token_status::create_status,					 "close", 			fsm_token_status::close_status, 			fsm_token_close::fsm_cond_close, fsm_token_close::fsm_action_close, fsm_token_close::fsm_exec_close},
	// restore funds
	token_fsm::fsm_entry{fsm_token_status::create_status, 		 	 "restore", 		fsm_token_status::restore_status,			fsm_token_restore::fsm_cond_restore, fsm_token_restore::fsm_action_restore, fsm_token_restore::fsm_exec_restore},
	token_fsm::fsm_entry{fsm_token_status::phase1_begin_status,  	 "restore", 		fsm_token_status::restore_status, 			fsm_token_restore::fsm_cond_restore, fsm_token_restore::fsm_action_restore, fsm_token_restore::fsm_exec_restore},
	token_fsm::fsm_entry{fsm_token_status::phase1_end_status, 	 	 "restore", 		fsm_token_status::restore_status, 			fsm_token_restore::fsm_cond_restore, fsm_token_restore::fsm_action_restore, fsm_token_restore::fsm_exec_restore},
	token_fsm::fsm_entry{fsm_token_status::phase2_begin_status,  	 "restore", 		fsm_token_status::restore_status, 			fsm_token_restore::fsm_cond_restore, fsm_token_restore::fsm_action_restore, fsm_token_restore::fsm_exec_restore},
	token_fsm::fsm_entry{fsm_token_status::phase2_end_status, 	 	 "restore", 		fsm_token_status::restore_status, 			fsm_token_restore::fsm_cond_restore, fsm_token_restore::fsm_action_restore, fsm_token_restore::fsm_exec_restore},
	
	token_fsm::fsm_entry{fsm_token_status::create_status,		     "set_control",		fsm_token_status::create_status, 			fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::phase1_begin_status,		 "set_control", 	fsm_token_status::phase1_begin_status, 		fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::phase1_end_status,		 "set_control", 	fsm_token_status::phase1_end_status, 		fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::phase2_begin_status, 	 "set_control", 	fsm_token_status::phase2_begin_status,		fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::phase2_end_status,		 "set_control", 	fsm_token_status::phase2_end_status,		fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::settle_status,			 "set_control", 	fsm_token_status::settle_status,			fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control}, 
	token_fsm::fsm_entry{fsm_token_status::return_asset_end_status,	 "set_control", 	fsm_token_status::return_asset_end_status, 	fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::restore_status, 	 		 "set_control", 	fsm_token_status::restore_status,			fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control},
	token_fsm::fsm_entry{fsm_token_status::close_status,		 	 "set_control", 	fsm_token_status::close_status,				fsm_token_set_control::fsm_cond_set_control, fsm_token_set_control::fsm_action_set_control, fsm_token_set_control::fsm_exec_set_control}
};

int token_fsm::fsm_do(database& db, const std::function<void(token_fsm_message&)>& constructor)
{ 
	bool exist_action = false;
	string msg = event_ + ": ";

	try {
		if(current_state_ == fsm_token_status::none_status)
			current_state_ = get_token(db, e_object.token_id).status; //return false;

		std::vector<struct fsm_entry>& fsm_entries = token_fsm::fsm_entries_;
		std::vector<fsm_entry>::iterator vIter = fsm_entries.begin();
		while(vIter != fsm_entries.end()) {

			optional<fsm_entry> entry = *vIter;

			// 1. current state and set event 
			if(!(entry->current_state == current_state_ && entry->event == event_)) {
				vIter++;
				continue;
			}

			// 2. trigger event condition
			if(!entry->cond(*this, db)) {
				vIter++;
				continue;
			}

			// 3. exec action
			if(entry->action(*this, db)) {
				next_state_ = entry->next_state;
				exist_action = true;
				msg += "ok";
			} else {
				// 4. exception???
				entry->exec(*this, db);
				msg += "exception"; // fail???
			}
			break;
			vIter++;
		}

  	} catch( const fc::exception& e ) {
      	msg += e.to_detail_string();
   	} catch( ... ) {
   		msg += "fatal-error";
   	}

	// 5. callback ok or not
	auto message = token_fsm_message{exist_action, msg};
	constructor(message);
	return exist_action;
}

int token_fsm::fsm_init()
{
	return true;
}
int token_fsm::fsm_term()
{
	return true;
}

//create
//判断
bool fsm_token_create::fsm_cond_create(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_create id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	
	token_object token =  get_token(db, fsm.get_token_id());

	if(fsm.get_time() >= token.create_time())
		return true; //create
	return false;
}
//成功处理
//if fsm_cond_create() return true, then invoke fsm_action_create()
bool fsm_token_create::fsm_action_create(token_fsm& fsm, database& db)
{
	ilog("fsm_action_create id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	// update token status
	auto& token = fsm.get_token_id()(db);

	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::create_status;
    });

	return true;
}

//失败处理
//if fsm_cond_create() return false, then invoke fsm_action_create()
bool fsm_token_create::fsm_exec_create(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_create id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}


//phase1_begin
bool fsm_token_phase1_begin::fsm_cond_phase1_begin(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_phase1_begin id=${id}, status=${current}, event~${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	if(fsm.event_test("test") != "")	return true;

	token_object token =  get_token(db, fsm.get_token_id());
	if(fsm.get_time() >= token.phase1_begin_time())
		return true; //phase1 begin
	return false;
}

bool fsm_token_phase1_begin::fsm_action_phase1_begin(token_fsm& fsm, database& db)
{
	ilog("fsm_action_phase1_begin id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	// update token status
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::phase1_begin_status;
    });

	return true;
}
bool fsm_token_phase1_begin::fsm_exec_phase1_begin(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_phase1_begin id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}

//phase1_end
bool fsm_token_phase1_end::fsm_cond_phase1_end(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_phase1_end id=${id}, status=${current}, event=${event}", 
			("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	if(fsm.event_test("test") != "")	return true;

	token_object token =  get_token(db, fsm.get_token_id());
	if(fsm.get_time() >= token.phase1_end_time())
		return true; //phase1 over
	return false;
}

bool fsm_token_phase1_end::fsm_action_phase1_end(token_fsm& fsm, database& db)
{
	ilog("fsm_action_phase1_end id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	// update token status
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::phase1_end_status;
    });

	return true;
}
bool fsm_token_phase1_end::fsm_exec_phase1_end(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_phase1_end id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}


//phase2_begin
bool fsm_token_phase2_begin::fsm_cond_phase2_begin(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_phase2_begin id=${id}, status=${current}, event~${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	if(fsm.event_test("test") != "")	return true;

	token_object token =  get_token(db, fsm.get_token_id());
	if(fsm.get_time() >= token.phase2_begin_time())
		return true;
	return false;
}

bool fsm_token_phase2_begin::fsm_action_phase2_begin(token_fsm& fsm, database& db)
{
	ilog("fsm_action_phase1_begin id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	// update token status
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::phase2_begin_status;
    });

	return true;
}

bool fsm_token_phase2_begin::fsm_exec_phase2_begin(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_phase2_begin id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}

//phase2_end
bool fsm_token_phase2_end::fsm_cond_phase2_end(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_phase2_end id=${id}, status=${current}, event=${event}", 
			("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	if(fsm.event_test("test") != "")	return true;

	token_object token =  get_token(db, fsm.get_token_id());
	if(fsm.get_time() >= token.phase2_end_time())
		return true;
	return false;
}

bool fsm_token_phase2_end::fsm_action_phase2_end(token_fsm& fsm, database& db)
{
	ilog("fsm_action_phase2_end id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));

    //判断募集是否成功
    auto& token = fsm.get_token_id()(db);
    auto& token_statistics = token.statistics(db);

	// update token status
	db.modify(token, [&](token_object& obj) 
	{
        obj.status = fsm_token_status::phase2_end_status;
        obj.result.is_succeed = token_statistics.actual_buy_total >= token.buy_succeed_min_amount ? true : false;
        //obj.status_expires.settle_time = obj.status_expires.phase2_end; // 实际结算时间=认购第2阶段结束时间
    });

	return true;
}
bool fsm_token_phase2_end::fsm_exec_phase2_end(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_phase2_end id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}

/*
 * return: (actual_not_buy_total*my_buy/actual_buy_total)
*/
share_type calclate_user_issued_asset_reward(share_type actual_not_buy_total, share_type my_buy, share_type actual_buy_total)
{
	if( actual_not_buy_total == 0 || my_buy == 0 || actual_buy_total == 0)
      return 0;

	fc::uint128 pool(actual_not_buy_total.value);
   	pool *= my_buy.value;
   	pool /= actual_buy_total.value;

   	return pool.to_uint64();
}


//结算
static bool settle_handle(database& db, const token_object& token)
{	
	auto& token_statistics = token.statistics(db);
	auto& token_buy = db.get_index_type<token_buy_index>().indices().get<by_token_id>();
	auto buy_itr = token_buy.lower_bound( token.id );
    auto buy_end = token_buy.upper_bound( token.id );
	FC_ASSERT( buy_itr != buy_end, "settle_handle token does not exist. id=${id}", ("id", token.id));

	share_type reward_amount = 0;
	//share_type total = 0;
	//asset result_user_issued_asset(0, token.user_issued_asset_id);

	while(buy_itr != buy_end) {
		const token_buy_object* buy_obj = &*buy_itr;
		db.modify(*buy_obj, [&](token_buy_object& buy)
		{
			if(token.template_parameter.not_buy_asset_handle == token_rule::not_buy_asset_handle_way::dispatch_to_buyer) //剩余的没认购用户资产按比例分发给已经参与的人
			{
				//计算每次认购可以分得的用户资产
				reward_amount = calclate_user_issued_asset_reward(token_statistics.actual_not_buy_total, buy.buy_result.buy_quote_amount.amount, token_statistics.actual_buy_total);
			}
			else//剩余的没认购用户资产直接销毁
			{
				reward_amount = 0;
			}
				
			//total = buy.buy_result.buy_quote_amount.amount + reward_amount;
			//result_user_issued_asset.amount = total;

			//update token_buy_object
			db.modify( *buy_obj, [&]( token_buy_object& buy )
			{
				buy.buy_result.reward_quote_amount = asset(reward_amount, token.user_issued_asset_id);
			});

			//update buyer balance
			db.adjust_balance(buy.buyer, buy.buy_result.buy_quote_amount + buy.buy_result.reward_quote_amount);
			
			// fee right ???
			if( buy.deferred_fee > 0 )
			{
				db.modify( buy.buyer(db).statistics(db), [&]( account_statistics_object& statistics )
				{
					statistics.pay_fee( buy.deferred_fee, db.get_global_properties().parameters.cashback_vesting_threshold );
				});
			}
		});
		buy_itr++;
	}

	if(token.template_parameter.not_buy_asset_handle == token_rule::not_buy_asset_handle_way::burn_asset) //剩余的没认购用户资产直接销毁
	{
		ilog("settle_handle total_burn=${burn}", ("burn", token_statistics.actual_not_buy_total));
		db.adjust_balance( GRAPHENE_NULL_ACCOUNT, asset(token_statistics.actual_not_buy_total, token.user_issued_asset_id));
	}

	return true;
}


bool fsm_token_settle::fsm_cond_settle(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_settle status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));
	if(fsm.event_test("test") != "") return true;

	token_object token =  get_token(db, fsm.get_token_id());
	//if(fsm.get_time() >= token.settle_time()) //这里不能通过时间来判断，因为这样手工触发事件时无法提前结算
	FC_ASSERT(token.result.is_succeed, "fsm_cond_settle token.result.is_succeed should be true, but now it is false");
	return true;

}


bool fsm_token_settle::fsm_action_settle(token_fsm& fsm, database& db)
{
	ilog("fsm_action_settle id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	try {
		auto& token = fsm.get_token_id()(db);
		
		// update token status and creator reward put out
		db.modify(token, [&](token_object& obj) {
	        //obj.status_expires.settle_time = db.head_block_time(); // 实际结算时间<=认购第2阶段结束时间
	        obj.status = fsm_token_status::settle_status;
	        //obj.result.is_succeed = true;
	    });

		//要先更新状态为settle_status再调用settle_handle()
	    settle_handle(db, token);

	} catch( const fc::exception& e ) {
		elog("fsm_action_settle id=${id}, status=${current}, event=${event}, err=${err}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event())("err", e.to_detail_string()));
		//return false;
		throw e;
   	} catch( ... ) {
		elog("fsm_action_settle id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
		throw;
		//return false;
   	}

   	ilog("fsm_action_settle is end. id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}

bool fsm_token_settle::fsm_exec_settle(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_settle status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));
/*	// update token status
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::return_asset_end_status;
    });*/
	return true;
}


bool fsm_token_return_asset_end::fsm_cond_return_asset_end(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_return_asset_end id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	if(fsm.event_test("test") != "")	return true;

	token_object token =  get_token(db, fsm.get_token_id());
	if(fsm.get_time() >= token.return_asset_end_time())
		return true; //phase1 over
	return false;
}


bool fsm_token_return_asset_end::fsm_action_return_asset_end(token_fsm& fsm, database& db)
{
	try
	{
		// handle return_asset_end result ***
		auto& token = fsm.get_token_id()(db);

		// update token status	
		db.modify(token, [&](token_object& obj) {
	        obj.status = fsm_token_status::return_asset_end_status;
	        obj.status_expires.return_asset_end = db.head_block_time();
	    });

    } catch( const fc::exception& e ) {
		elog("fsm_action_return_asset_end status=${current} event=${event} ~${err}", ("current", fsm.get_current())("event", fsm.get_event())("err", e.to_detail_string()));
		throw e; //return false;
   	} catch( ... ) {
		elog("fsm_action_return_asset_end status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));
		throw;
		//return false;
   	}

	return true;
}

bool fsm_token_return_asset_end::fsm_exec_return_asset_end(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_return_asset_end status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));
/*	// update token status
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::phase1_end_status;
    });*/
	return true;
}

bool fsm_token_close::fsm_cond_close(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_close status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));

	return true;
}

bool fsm_token_close::fsm_action_close(token_fsm& fsm, database& db)
{
	ilog("fsm_action_close status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));
	// update token status
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::close_status;
    });

	return true;
}
bool fsm_token_close::fsm_exec_close(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_close status=${current} event=${event}", ("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}



//回滚
//当token状态为settle_status|return_asset_end_status|close_status不能调用restore_handle
static bool restore_handle(database& db, const token_object& token)
{
	auto& token_statistics = token.statistics(db);
	ilog("token restore_handle id=${id} status=${status}", ("id", token.id)("status", token.status));

/*	//发行人预留的用户资产退回给发行人
	if(token.issuer_reserved_asset_total > 0 && token.template_parameter.issuer_reserved_asset_frozen_months == 0)
	{
		db.adjust_balance(token.issuer, asset(std::stoll(token.template_parameter.plan_buy_total), token.user_issued_asset_id));
	}
	else
	{
		db.adjust_balance(token.issuer, asset(std::stoll(token.template_parameter.max_supply), token.user_issued_asset_id));
	}
*/
	//所有的的通证(用户资产)退回给发行人
	db.adjust_balance(token.issuer, asset(std::stoll(token.template_parameter.max_supply), token.user_issued_asset_id));

	//抵押的核心资产退回给发行人
	db.adjust_balance(token.issuer, token.template_parameter.guaranty_core_asset_amount);

	db.modify(token_statistics, [&](token_statistics_object& obj) {
			obj.has_returned_guaranty_core_asset += token.template_parameter.guaranty_core_asset_amount;
			obj.return_guaranty_core_asset_detail.push_back(return_asset_record(db.head_block_time(), token.template_parameter.guaranty_core_asset_amount));
    });

	//处理认购回滚
	auto& token_buy = db.get_index_type<token_buy_index>().indices().get<by_token_id>();
	auto buy_itr = token_buy.lower_bound( token.id );
    auto buy_end = token_buy.upper_bound( token.id );
	//FC_ASSERT( buy_itr != buy_end );//可以没有任何认购

	while(buy_itr != buy_end)
	{
		//认购金额(核心资产)退回给认购人
		db.adjust_balance(buy_itr->buyer, buy_itr->buy_result.pay_base_amount);

		//认购手续费(核心资产)退回给认购人
		if( buy_itr->deferred_fee > 0 )
			db.adjust_balance(buy_itr->buyer, buy_itr->deferred_fee);

		++buy_itr;
	}

	return true;
}

bool fsm_token_restore::fsm_cond_restore(token_fsm& fsm, database& db)
{
	ilog("fsm_token_restore::fsm_cond_restore id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	token_object token =  get_token(db, fsm.get_token_id());
	if(token.enable_restore())
		return true;
	return true;
}

bool fsm_token_restore::fsm_action_restore(token_fsm& fsm, database& db)
{
	ilog("fsm_token_restore::fsm_action_restore id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	auto& token = fsm.get_token_id()(db);

	// update token status
	db.modify(token, [&](token_object& obj) {

        obj.status = fsm_token_status::restore_status;
        obj.result.is_succeed = false;
    });

	//要先更新状态为restore_status再调用restore_handle()
	restore_handle(db, token);
	ilog("fsm_token_restore::fsm_action_restore is end. id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}


bool fsm_token_restore::fsm_exec_restore(token_fsm& fsm, database& db)
{
	ilog("fsm_token_restore::fsm_exec_restore id=${id}, status=${current} event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}


//set_control
bool fsm_token_set_control::fsm_cond_set_control(token_fsm& fsm, database& db)
{
	ilog("fsm_cond_set_control id=${id}, status=${current}, event~${event}, content=${content}", 
			("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event())("content", fsm.get_event_content()));

	int i = std::stoi(fsm.get_event_content());

	if( i < (int)(graphene::chain::token_object::token_control::available) || i > (int)(graphene::chain::token_object::token_control::end_of_token_control) )
	{
		ilog("fsm_cond_set_control return false. id=${id}, status=${current}, event~${event}, content=${content}", 
			("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event())("content", fsm.get_event_content()));
		return false;
	}
	return true;
}

bool fsm_token_set_control::fsm_action_set_control(token_fsm& fsm, database& db)
{
	ilog("fsm_action_set_control id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	// update token control
	auto& token = fsm.get_token_id()(db);
	db.modify(token, [&](token_object& obj) {

        obj.control = (graphene::chain::token_object::token_control)std::stoi(fsm.get_event_content());
    });

	return true;
}

bool fsm_token_set_control::fsm_exec_set_control(token_fsm& fsm, database& db)
{
	ilog("fsm_exec_set_control id=${id}, status=${current}, event=${event}", ("id", fsm.get_token_id())("current", fsm.get_current())("event", fsm.get_event()));
	return true;
}


bool database::apply_token_event(const token_event_object& event_object)
{
	token_fsm(*this, event_object).fsm_do(*this, [&](token_fsm_message& msg) {

		modify(event_object, [&](token_event_object& obj) {
			// return handle result
	        obj.status		= msg.status;
	        obj.message		= msg.msg;
	    });

	});

	return true;
}

//认购
object_id_type database::apply_token_buy(const token_buy_operation& o, const share_type& deferred_fee)
{
	auto& token = o.token_id(*this);
	auto& token_statistics = token.statistics(*this);
	share_type pay_amount = 0;
    share_type buy_amount = 0;
    share_type quote_amount_for_each_buy = 0;

	const auto& new_token_buy_object = create<token_buy_object>( [&]( token_buy_object& obj )
	{
    	obj.buyer       		= o.buyer;
    	obj.token_id    		= o.token_id;
		obj.buy_time 			= head_block_time();
    	obj.template_parameter  = o.template_parameter;

    	if(o.template_parameter.phase == token_buy_template::token_buy_phase::buy_phase1)//认购阶段1
    	{
    		FC_ASSERT(token.template_parameter.buy_phases.find("1") != token.template_parameter.buy_phases.end(), "can't find buy phase 1. token id=${id}", ("id", token.id));
    		//base
    		pay_amount = o.get_buy_quantity() * token.template_parameter.buy_phases.find("1")->second.quote_base_ratio.base.amount;
    		obj.buy_result.pay_base_amount	= asset(pay_amount, asset_id_type());
    		//quote
    		buy_amount = o.get_buy_quantity() * token.template_parameter.buy_phases.find("1")->second.quote_base_ratio.quote.amount;
    		obj.buy_result.buy_quote_amount	= asset(buy_amount, token.user_issued_asset_id);

    		quote_amount_for_each_buy = token.template_parameter.buy_phases.find("1")->second.quote_base_ratio.quote.amount;
    	}
    	else//认购阶段2
    	{
    		FC_ASSERT(token.template_parameter.buy_phases.find("2") != token.template_parameter.buy_phases.end(), "can't find buy phase 2. token id=${id}", ("id", token.id));
    		//base
    		pay_amount = o.get_buy_quantity() * token.template_parameter.buy_phases.find("2")->second.quote_base_ratio.base.amount;
    		obj.buy_result.pay_base_amount	= asset(pay_amount, asset_id_type());
    		//quote
    		buy_amount = o.get_buy_quantity() * token.template_parameter.buy_phases.find("2")->second.quote_base_ratio.quote.amount;
    		obj.buy_result.buy_quote_amount	= asset(buy_amount, token.user_issued_asset_id);

    		quote_amount_for_each_buy = token.template_parameter.buy_phases.find("2")->second.quote_base_ratio.quote.amount;
    	}
    	
    	obj.buy_result.reward_quote_amount = asset(0, token.user_issued_asset_id);
    	obj.deferred_fee  	    = deferred_fee;
  	});

	//adjust buyer balance
	adjust_balance(o.buyer, -asset(pay_amount, asset_id_type()));

	//update statistics
	modify(token_statistics, [&](token_statistics_object& dyn) 
	{
/*		if(dyn.buyer_ids.empty())
		{
			dyn.buyer_ids.push_back(o.buyer);
		}
		else
		{
			vector<account_id_type>::iterator iter;
			for(iter = dyn.buyer_ids.begin(); iter != dyn.buyer_ids.end(); ++ iter )
			{
				if(*iter == o.buyer)
					break;
			}
 
			if(iter == dyn.buyer_ids.end())
			{
        		dyn.buyer_ids.insert(o.buyer);
        	}
		}
*/
		dyn.buyer_ids.insert(o.buyer);
		dyn.buyer_number			 = dyn.buyer_ids.size();
		dyn.actual_core_asset_total  += pay_amount;
		dyn.actual_buy_total 		 += buy_amount;
		//为防止溢出，actual_buy_total和plan_buy_total先去掉小数部分
		//dyn.actual_buy_percentage 	 = ((dyn.actual_buy_total/GRAPHENE_BLOCKCHAIN_PRECISION) * GRAPHENE_100_PERCENT) / (std::stoll(token.template_parameter.plan_buy_total)/GRAPHENE_BLOCKCHAIN_PRECISION); //保留2位小数。如值为1234，表示12.34%
		dyn.actual_buy_percentage 	 = ((dyn.actual_buy_total/GRAPHENE_BLOCKCHAIN_PRECISION) * GRAPHENE_100_PERCENT) / (token.buy_succeed_min_amount / GRAPHENE_BLOCKCHAIN_PRECISION); //保留2位小数。如值为1234，表示12.34%
		dyn.actual_not_buy_total 	 = std::stoll(token.template_parameter.plan_buy_total) - dyn.actual_buy_total;
	});

	//如果实际认购的总用户资产达到计划募集的用户资产数量，或者剩余的可认购用户资产数量小于1份认购里的用户资产数量，则认购提前结束
	if( token_statistics.actual_buy_total >= std::stoll(token.template_parameter.plan_buy_total) ||
		token_statistics.actual_not_buy_total < quote_amount_for_each_buy )
	{
		ilog("token_id=${id}, plan_buy_total has been reached. token_statistics.actual_buy_total=${actual_buy_total}", ("id", token.id)("actual_buy_total", token_statistics.actual_buy_total));

		modify(token, [&](token_object& obj) 
		{
	        //obj.status = fsm_token_status::phase2_end_status;
	        obj.result.is_succeed = true;
	        obj.status_expires.settle_time = head_block_time(); // 实际结算时间<=认购第2阶段结束时间

       		//下面的需要等待募集结束(结算)时才能最终确定，先定义为开始计算时间为众筹第2阶段结束时间，如果募集提前结束，再更新
			if( token.template_parameter.guaranty_core_asset_amount.amount > 0 && token.template_parameter.guaranty_core_asset_months > 0)
			{
				//发行人抵押的核心资产(AGC)是分期返还
				obj.status_expires.next_return_guaranty_core_asset_time = obj.status_expires.settle_time + SECONDS_OF_ONE_MONTH; //1个月按30天算，1 month = 2592000 seconds
				obj.status_expires.return_guaranty_core_asset_end = obj.status_expires.settle_time + token.template_parameter.guaranty_core_asset_months * SECONDS_OF_ONE_MONTH;
			}
			else
			{
				obj.status_expires.next_return_guaranty_core_asset_time = head_block_time();
				obj.status_expires.return_guaranty_core_asset_end = head_block_time();
			}
			
			if(token.issuer_reserved_asset_total > 0)
			{
				if(token.template_parameter.issuer_reserved_asset_frozen_months == 0)//立刻一次性返还
					obj.status_expires.next_return_issuer_reserved_asset_time = obj.status_expires.settle_time;
				else
					obj.status_expires.next_return_issuer_reserved_asset_time = obj.status_expires.settle_time + SECONDS_OF_ONE_MONTH; //1个月按30天算，1 month = 2592000 seconds

				obj.status_expires.return_issuer_reserved_asset_end = obj.status_expires.settle_time + token.template_parameter.issuer_reserved_asset_frozen_months * SECONDS_OF_ONE_MONTH;
			}

			//如果募集提前结束，更新返还众筹抵押的核心资产(AGC)和发行人预留的用户资产结束时间
			if(token.status_expires.phase2_end > head_block_time())
			{
				obj.status_expires.return_asset_end = 
					obj.status_expires.return_guaranty_core_asset_end >= obj.status_expires.return_issuer_reserved_asset_end ? obj.status_expires.return_guaranty_core_asset_end : obj.status_expires.return_issuer_reserved_asset_end;				
			}
	    });

	    token_event_operation event_op;
	    transaction_evaluation_state event_context(this);

      	event_op.oper         	= GRAPHENE_COMMITTEE_ACCOUNT;
      	event_op.token_id     	= token.id;
      	event_op.event 			= "settle";
		event_op.fee = current_fee_schedule().calculate_fee( event_op );

		event_context.skip_fee_schedule_check = true;
		ilog("database::apply_token_buy token_id=${id}, time=${time}, event=${event}", ("id", token.id)("time", head_block_time())("event", event_op.event));
		event_op.fee = current_fee_schedule().calculate_fee( event_op );

		event_context.skip_fee_schedule_check = true;

		try {
		// inner undo session for not interupt block handle
		auto session = _undo_db.start_undo_session(true);
		apply_operation(event_context, event_op);
		session.merge();

		} catch (const fc::exception& e) {
			elog( "<apply_token_buy> ${e}", ("e",e.to_detail_string() ) );
      		//throw;
      		return new_token_buy_object.id;;
		}
	}

	return new_token_buy_object.id;
}


int database::token_transition()
{
	//static uint8_t exec_run = 0;
	transaction_evaluation_state event_context(this);
	auto& token_indexs = get_index_type<token_index>().indices().get<by_id>();
	auto iter = token_indexs.begin();
	//auto iter 		= token_indexs.lower_bound( head_block_time() );

    //auto token_end = token_indexs.upper_bound( head_block_time() );
    auto token_end = token_indexs.end();
    //const chain_parameters& chain_parameters = get_global_properties().parameters;

    const uint number_of_handle_token_per_block = 30; //每个出块周期最多处理的众筹项目的个数
    static uint64_t handle_begin_index = 1; //从整个集合的第一个众筹项目开始处理，每个出块周期会记录下个周期一开始要处理的众筹项目的记录位置
    
//    uint64_t total_token = token_indexs.size();
    
    uint64_t handle_count = 0; //本出块周期已处理的众筹项目的个数
    uint64_t index = 1;
    bool skip = true;

	while(iter != token_end && handle_count < number_of_handle_token_per_block ) 
	{
		if(skip && index < handle_begin_index) //先跳到本出块周期一开始要处理的众筹项目的记录位置
      	{
      		++index;
      		++iter;
      		continue;
      	}
      	else if (skip)
      	{
      		skip = false;
      	}

		const token_object& token = *iter;
		auto& token_statistics = token.statistics(*this);

		// according to token_object status & event time, will trigger token_event_operation, with synchronization to token_fsm
		token_event_operation event_op;
      	event_op.oper         = GRAPHENE_COMMITTEE_ACCOUNT;
      	event_op.token_id     = token.id;

      	bool bContinue = false;

      	switch(token.status) 
      	{
	      	case token_object::token_status::none_status : {bContinue = true; } break;
	      	case token_object::token_status::create_status :
	      	{
	      		if(token.phase1_begin_time() <= head_block_time())
	      		{
		      		event_op.event 		= "phase1_begin";
				}
				else
				{
					bContinue = true;
				} 
	      	}break;
	      	case token_object::token_status::phase1_begin_status: 
	      	{
	      		if(token.phase1_end_time() <= head_block_time()) 
	      		{
	      			event_op.event 		= "phase1_end";
				} 
				else
				{
					bContinue = true;
				} 
	      	} break;

	      	case token_object::token_status::phase1_end_status : 
	      	{
	      		if(token.phase2_begin_time() <= head_block_time())
	      		{
		      		event_op.event 		= "phase2_begin";
				} 
				else
				{
					bContinue = true;	
				} 
	      	} break;

	      	case token_object::token_status::phase2_begin_status: 
	      	{
	      		if(token.phase2_end_time() <= head_block_time()) 
	      		{
	      			event_op.event 		= "phase2_end";
				} 
				else
				{
					bContinue = true;
				} 
	      	} break;

	      	case token_object::token_status::phase2_end_status : 
	      	{
				if( token.result.is_succeed )//募集成功，转settle状态
			    {
			    	event_op.event = "settle";
			    }
				else//募集不成功，转restore状态
				{
					event_op.event = "restore";
				}
	      	} break;

	      	case token_object::token_status::settle_status :
	      	{
	      		// 返还众筹抵押的核心资产(AGC)和发行人预留的用户资产结束
	      		if( ( token.template_parameter.guaranty_core_asset_amount.amount <= 0 || 
	      			  (token.template_parameter.guaranty_core_asset_amount.amount > 0 && token.template_parameter.guaranty_core_asset_months == 0 && token_statistics.return_guaranty_core_asset_detail.size() == 1) || 
	      			  (token.template_parameter.guaranty_core_asset_amount.amount > 0 && token.template_parameter.guaranty_core_asset_months > 0 && token_statistics.return_guaranty_core_asset_detail.size() == token.template_parameter.guaranty_core_asset_months)
	      			) // 核心资产(AGC)
					&& 
	      		    ( token.issuer_reserved_asset_total <= 0 || 
	      		      (token.issuer_reserved_asset_total > 0 && token.template_parameter.issuer_reserved_asset_frozen_months ==0 && token_statistics.return_issuer_reserved_asset_detail.size() == 1) ||
	      		      (token.issuer_reserved_asset_total > 0 && token.template_parameter.issuer_reserved_asset_frozen_months > 0 && token_statistics.return_issuer_reserved_asset_detail.size() == token.template_parameter.issuer_reserved_asset_frozen_months)
	      		    ) // 用户资产
	      		  )
	      		{
	      			event_op.event = "return_asset_end";
	      			break;
	      		}

	      		//不需要产生event
	      		bContinue = true;

	      		//返还发行人抵押的核心资产(AGC)没结束
	      		if( token.template_parameter.guaranty_core_asset_amount.amount > 0 && 
	      			token.next_return_guaranty_core_asset_time() <= head_block_time() && 
	      			( (token.template_parameter.guaranty_core_asset_months == 0 && token_statistics.return_guaranty_core_asset_detail.size() <= 0 ) ||  //如果发行人抵押的核心资产(AGC)是一次性返还
	      			  (token.template_parameter.guaranty_core_asset_months > 0 && token_statistics.return_guaranty_core_asset_detail.size() < token.template_parameter.guaranty_core_asset_months)//如果发行人抵押的核心资产(AGC)是分期返还
	      			)
	      		  )
	      		{
	      			//返还当期应返还的发行人抵押的核心资产
	      			//update issuer balance
	      			asset core_asset;
	      			uint8_t is_last = false; //是不是最后一次返还

	      			if(token.template_parameter.guaranty_core_asset_months == 0)//一次性返还
	      			{
	      				core_asset = token.template_parameter.guaranty_core_asset_amount;
	      				is_last = true;
	      			}
	      			else//分期返还
	      			{
	      				if(token.template_parameter.guaranty_core_asset_months - token_statistics.return_guaranty_core_asset_detail.size() == 1)
	      					is_last = true;

		      			if( is_last )//如果是最后一期
		      			{
		      				share_type diff = token.template_parameter.guaranty_core_asset_amount.amount - token_statistics.has_returned_guaranty_core_asset.amount;
		      				core_asset = asset(diff, asset_id_type());
		      			}
		      			else
		      				core_asset = asset(token.each_period_return_guaranty_core_asset, asset_id_type());	      				
	      			}

					adjust_balance(token.issuer, core_asset);

					modify(token_statistics, [&](token_statistics_object& obj) {
	        					//obj.return_guaranty_core_asset_detail[1] = core_asset;
								obj.has_returned_guaranty_core_asset += core_asset;
								obj.return_guaranty_core_asset_detail.push_back(return_asset_record(head_block_time(), core_asset));
					});

	      			modify(token, [&](token_object& obj) {
						if( !is_last )//不是最后一次返还
						{
							obj.status_expires.next_return_guaranty_core_asset_time = (head_block_time() + SECONDS_OF_ONE_MONTH); //1个月按30天算，1 month = 2592000 seconds;
						}
						else//是最后一次返还
						{
							obj.guaranty_credit = 0;
						}
					});	
	      		}

	      		//返还发行人预留的用户资产结束没结束
	      		if( token.issuer_reserved_asset_total > 0 && 
	      			token.next_return_issuer_reserved_asset_time() <= head_block_time() && 
	      			( (token.template_parameter.issuer_reserved_asset_frozen_months == 0 && token_statistics.return_issuer_reserved_asset_detail.size() <= 0) || //如果发行人预留的用户资产(通证)是一次性返还
	      			  (token.template_parameter.issuer_reserved_asset_frozen_months > 0 && token_statistics.return_issuer_reserved_asset_detail.size() < token.template_parameter.issuer_reserved_asset_frozen_months)//如果发行人预留的用户资产(通证)是分期返还
	      			)
	      		  )
	      		{
	      			//返还当期应返还的发行人预留的用户资产
	      			//update issuer balance
	      			asset user_issued_asset;
	      			uint8_t is_last = false; //是不是最后一次返还

	      			if(token.template_parameter.issuer_reserved_asset_frozen_months == 0)//一次性返还
	      			{
	      				user_issued_asset = asset(token.issuer_reserved_asset_total, token.user_issued_asset_id);
	      				is_last = true;
	      			}
	      			else//分期返还
	      			{
	      				if(token.template_parameter.issuer_reserved_asset_frozen_months - token_statistics.return_issuer_reserved_asset_detail.size() == 1)
	      					is_last = true;

		      			if( is_last)//如果是最后一次返还
		      			{
     						share_type diff = token.issuer_reserved_asset_total - token_statistics.has_returned_issuer_reserved_asset.amount;
		      				user_issued_asset = asset(diff, token.user_issued_asset_id);
		      			}
		      			else
		      				user_issued_asset = asset(token.each_period_return_issuer_reserved_asset, token.user_issued_asset_id);
		      		}

					adjust_balance(token.issuer, user_issued_asset);

					modify(token_statistics, [&](token_statistics_object& obj) {
							obj.has_returned_issuer_reserved_asset += user_issued_asset;
	        				obj.return_issuer_reserved_asset_detail.push_back(return_asset_record(head_block_time(), user_issued_asset));
					});

					modify(token, [&](token_object& obj) {
  						if( !is_last )//不是最后一次返还
  						{
  							obj.status_expires.next_return_issuer_reserved_asset_time = (head_block_time() + SECONDS_OF_ONE_MONTH); //1个月按30天算，1 month = 2592000 seconds;
  						}
					});
	      		}
	      	}break;

	      	case token_object::token_status::return_asset_end_status :
	      	case token_object::token_status::close_status :
	      	case token_object::token_status::restore_status : {bContinue = true; } break;
      	}

		++iter;
      	++handle_begin_index;
      	++handle_count;
	    if (iter == token_end)
	    {
	    	handle_begin_index = 1;//下个出块周期从第一个众筹项目开始处理
	    }

		if(bContinue) { // nothing event handle
			continue;
		}
		//--exec_run; // read run count

		ilog("token expire id=${token}, time=${time} now=${now}, status=${status}, event=${event}", ("token", iter->id)("time", iter->create_time())("now", head_block_time())("status", token.status)("event", event_op.event));
		event_op.fee = current_fee_schedule().calculate_fee( event_op );

		event_context.skip_fee_schedule_check = true;

		try {
		// inner undo session for not interupt block handle
		auto session = _undo_db.start_undo_session(true);
		apply_operation(event_context, event_op);
		session.merge();
		} catch (const fc::exception& e) {
			elog( "<token_transition> ${e}", ("e",e.to_detail_string() ) );
      		//throw;
      		return 1;
		}
	} //while
	return 0;
}

void database::expire_token_event()
{ try {
	detail::with_skip_flags(*this, 
		get_node_properties().skip_flags | skip_authority_check, [&](){

		//try {

			// inner undo session for not interupt block handle
			//auto session = _undo_db.start_undo_session();
			token_transition();
			//session.merge();

		//} catch (const fc::exception& e) {
		//	elog( "<expire_token_event> e", ("e",e.to_detail_string() ) );
      		//throw;
		//}
		
	});
} FC_CAPTURE_AND_RETHROW( () ) }



} } // graphene::chain
