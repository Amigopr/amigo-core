/*
 * Copyright (c) 2017 Amigo, Inc., and contributors.
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
#include <graphene/chain/module_configurator.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/module_cfg_object.hpp>
#include <graphene/chain/word_object.hpp>
#include <graphene/chain/operation_blacklist_object.hpp>
#include <fc/variant_object.hpp>
#include <fc/variant.hpp>

#include <functional>

namespace graphene { namespace chain {


const string token_configurator::name = TOKEN_MODULE_NAME;
const string word_configurator::name = WORD_MODULE_NAME;
const string operation_blacklist_configurator::name = OPERATION_BLACKLIST_MODULE_NAME;


// 校验字符串是不是数字
bool isNum(string str)  
{  
    for(uint64_t i=0;i<str.length();++i)
	{
	  if(str[i] >= '0' && str[i] <= '9')
	  {
	      continue;
	  }
	  else
	  {
	      //printf("i=%d, str[i]=%c\n", i, str[i]);
	      return false;
	  }
	}

	return true;
}

/**********************************************************************
example for "TOKEN" module_cfg_object

{
  "id": "1.20.1",
  "module_name": "TOKEN",
  "module_cfg": {
  	"name_length": {"min":3, "max":32},
    "symbol_length": {"min":3, "max":6},
    "token_brief_length": {"min":0, "max":120},
    "token_description_length": {"min":0, "max":10240},
    "max_days_between_create_and_phase1_begin": 90, 
    "collect_days": {"min":3, "max":90},
    "token_types": {"default1":"默认1", "default2":"默认2"},
    "token_subtypes": {"default1":{"subtype1":"子类型1", "subtype2":"子类型2"}, "default2":{"subtype3":"子类型3", "subtype4":"子类型4"}}, "default2":{"subtype5":"子类型5", "subtype6":"子类型6"}},
    "guaranty_core_asset_months": {"min":1, "max":36},
    "each_buy_core_asset_range": {"min":"100000000", "max":"100000000000000"}, //这里如果使用整数，只支持32位整数，所以这里使用字符串
    "issuer_reserved_asset_frozen_months": {"min":1, "max":36},
    "buy_max_times": 10,
    "reserved_asset_names": ["BitCoin"],
    "reserved_asset_symbols": ["BTC","ETH","EOS"]
  },
  "pending_module_cfg": {},
  "last_update_time": "2018-2-22T15:00:00",
  "last_modifier": "1.2.6"
}

**********************************************************************/
void_result token_configurator::evaluate(const module_cfg_operation& o)
{
	FC_ASSERT(name == o.module_name, "module name not matched. Mine: ${myname}, passed: ${passed}",
			("myname", name)("passed", o.module_name));

	if ( o.op_type != module_cfg_op_delete)
	{
		const variant_object& value = o.cfg_value;
		variant_object subvalue;

		//token_types
		if(value.contains("token_types"))
		{
			FC_ASSERT( value["token_types"].is_object(), "token_types should be an object, token_types=${token_types}", ("token_types", value["token_types"]));
			const variant_object& types = value["token_types"].get_object();

			for (auto itr_for_one_type = types.begin(); itr_for_one_type != types.end(); ++itr_for_one_type)
			{
				const string& type_en = itr_for_one_type->key();
				FC_ASSERT( itr_for_one_type->value().is_string(), "the value of key ${type_en} should be a string", ("type_en", type_en));
			}
		}

		//token_subtypes
		//if( !value["token_subtypes"].is_null() )
		if(value.contains("token_subtypes"))
		{
			FC_ASSERT( value["token_subtypes"].is_object(), "token_subtypes should be an object. token_subtypes=${token_subtypes}", ("token_subtypes", value["token_subtypes"]));
			const variant_object& subtypes = value["token_subtypes"].get_object();

			for (auto itr_for_one_type = subtypes.begin(); itr_for_one_type != subtypes.end(); ++itr_for_one_type)
			{
				const string& type_en = itr_for_one_type->key();

				//检查key(即token_type是否存在)

				bool is_existed = false;
				if(value.contains("token_types")) //新增的token_types
				{
					const variant_object& new_types = value["token_types"].get_object();
					for (auto itr_for_one_type_2 = new_types.begin(); itr_for_one_type_2 != new_types.end(); ++itr_for_one_type_2)
					{
						if(type_en == itr_for_one_type_2->key())
						{
							is_existed = true;
							break;
						}
					}					
				}

				if( !is_existed ) //原来的配置的token_types
				{
					module_cfg_object old_cfg = _db.get_module_cfg(token_configurator::name);

					if(old_cfg.module_cfg.contains("token_types"))
					{
						const variant_object& old_types = old_cfg.module_cfg["token_types"].get_object();
						for (auto itr_for_one_type_3 = old_types.begin(); itr_for_one_type_3 != old_types.end(); ++itr_for_one_type_3)
						{
							if(type_en == itr_for_one_type_3->key())
							{
								is_existed = true;
								break;
							}
						}					
					}					
				}

				FC_ASSERT( is_existed, "the token type corresponding to token subtype does not exist. type=${type}", ("type", type_en));

				const variant_object& subtypes = itr_for_one_type->value().get_object();
				for (auto itr_for_one_subtype = subtypes.begin(); itr_for_one_subtype != subtypes.end(); ++itr_for_one_subtype)
				{
					FC_ASSERT( itr_for_one_subtype->value().is_string(), "the value of subtype should be a string. type=${type}, subtype=${subtype}", 
							("type", itr_for_one_type->key())("subtype", itr_for_one_subtype->key()));
				}
			}
		}

		//asset_name_length
		if(value.contains("name_length"))
		{
			FC_ASSERT( value["name_length"].is_object(), "name_length should be an object. name_length=${name_length}", ("name_length", value["name_length"]));
			subvalue = value["name_length"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "name_length subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "name_length subfield max should be integer. max=${max}", ("max", subvalue["max"]));
		}

		//asset_symbol_length
		if(value.contains("symbol_length"))
		{
			FC_ASSERT( value["symbol_length"].is_object(), "symbol_length should be an object. asset_symbol_length=${symbol_length}", ("symbol_length", value["symbol_length"]));
			subvalue = value["symbol_length"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "symbol_length subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "symbol_length subfield max should be integer. max=${max}", ("max", subvalue["max"]));
		}

		//token_brief_length
		if(value.contains("brief_length"))
		{
			FC_ASSERT( value["brief_length"].is_object(), "token_brief_length should be an object. token_brief_length=${brief_length}", ("brief_length", value["brief_length"]));
			subvalue = value["brief_length"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "brief_length subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "brief_length subfield max should be integer. max=${max}", ("max", subvalue["max"]));
		}

		//token_description_length
		if(value.contains("description_length"))
		{
			FC_ASSERT( value["description_length"].is_object(), "token_description_length should be an object. description_length=${description_length}", ("description_length", value["description_length"]));
			subvalue = value["description_length"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "description_length subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "description_length subfield max should be integer. max=${max}", ("max", subvalue["max"]));
		}

		// max_days_between_create_and_phase1_begin
		if(value.contains("max_days_between_create_and_phase1_begin"))
		{
			FC_ASSERT( value["max_days_between_create_and_phase1_begin"].is_uint64(), "max_days_between_create_and_phase1_begin should be an integer. max_days_between_create_and_phase1_begin=${m}", 
					("m", value["max_days_between_create_and_phase1_begin"]));
		}

		//collect_days
		if(value.contains("collect_days"))
		{
			FC_ASSERT( value["collect_days"].is_object(), "collect_days should be an object. collect_days=${collect_days}", ("collect_days", value["collect_days"]));
			subvalue = value["collect_days"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "collect_days subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "collect_days subfield max should be integer. max=${max}", ("max", subvalue["max"]));
		}

		//guaranty_core_asset_months
		if(value.contains("guaranty_core_asset_months"))
		{
			FC_ASSERT( value["guaranty_core_asset_months"].is_object(), "collect_days should be an object. guaranty_core_asset_months=${g}", ("g", value["guaranty_core_asset_months"]));
			subvalue = value["guaranty_core_asset_months"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "guaranty_core_asset_months subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "guaranty_core_asset_months subfield max should be integer. max=${max}", ("max", subvalue["max"]));
		}

		//each_buy_core_asset_range
		if(value.contains("each_buy_core_asset_range"))
		{
			FC_ASSERT( value["each_buy_core_asset_range"].is_object(), "each_buy_core_asset_range should be an object. each_buy_core_asset_range=${g}", ("g", value["each_buy_core_asset_range"]));
			subvalue = value["each_buy_core_asset_range"].get_object();
			FC_ASSERT( subvalue["min"].is_string(), "each_buy_core_asset_range subfield min should be a string. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_string(), "each_buy_core_asset_range subfield max should be a string. max=${max}", ("max", subvalue["max"]));
		}

		//whitelist_max_size
		if(value.contains("whitelist_max_size"))
		{
			FC_ASSERT( value["whitelist_max_size"].is_uint64(), "whitelist_max_size should be integer. max=${max}", ("max", value["whitelist_max_size"]));
		}

		//issuer_reserved_asset_frozen_months
		if(value.contains("issuer_reserved_asset_frozen_months"))
		{
			FC_ASSERT( value["issuer_reserved_asset_frozen_months"].is_object(), "issuer_reserved_asset_frozen_months should be an object. issuer_reserved_asset_frozen_months=${g}", ("g", value["issuer_reserved_asset_frozen_months"]));
			subvalue = value["issuer_reserved_asset_frozen_months"].get_object();
			FC_ASSERT( subvalue["min"].is_uint64(), "issuer_reserved_asset_frozen_months subfield min should be integer. min=${min}", ("min", subvalue["min"]));
			FC_ASSERT( subvalue["max"].is_uint64(), "issuer_reserved_asset_frozen_months subfield max should be integer. max=${max}", ("max", subvalue["max"]));			
		}

		//buy_max_times
		if(value.contains("buy_max_times"))
		{
			FC_ASSERT( value["buy_max_times"].is_uint64(), "buy_max_times is not a number, buy_max_times=${buy_max_times}", ("buy_max_times", value["buy_max_times"]));
		}

		//reserved_asset_name
		if(value.contains("reserved_asset_names"))
		{
			FC_ASSERT( value["reserved_asset_names"].is_array(), "reserved_asset_name should be an array, reserved_asset_name=${reserved_asset_name}", ("reserved_asset_name", value["reserved_asset_name"]));

			const fc::variants& asset_names = value["reserved_asset_names"].get_array();
			for (auto itr_for_one_asset_name = asset_names.begin(); itr_for_one_asset_name != asset_names.end(); ++itr_for_one_asset_name)
			{
				FC_ASSERT(itr_for_one_asset_name->is_string(), "asset_name should be a string. asset_name=${asset_name}", ("asset_name", *itr_for_one_asset_name));
			}
		}
		
		//reserved_asset_symbol
		if(value.contains("reserved_asset_symbols"))
		{
			FC_ASSERT( value["reserved_asset_symbols"].is_array(), "reserved_asset_symbol should be an array, reserved_asset_symbol=${r}", ("r", value["reserved_asset_symbol"]));
			const fc::variants& asset_symbols = value["reserved_asset_symbols"].get_array();
			for (auto itr_for_one_asset_symbol = asset_symbols.begin(); itr_for_one_asset_symbol != asset_symbols.end(); ++itr_for_one_asset_symbol)
			{
				FC_ASSERT(itr_for_one_asset_symbol->is_string(), "asset_symbol should be a string. asset_symbol=${asset_symbol}", ("asset_symbol", *itr_for_one_asset_symbol));

				string asset_symbol = itr_for_one_asset_symbol->get_string();
				unsigned int len = asset_symbol.size();
				char c = 0;
				for(unsigned int i = 0; i < len; i++) //只允许大写字母
				{
					c = asset_symbol[i];
					FC_ASSERT( c >= 'A' && c <= 'Z', "asset_symbol can only contain A-Z. asset_symbol=${asset_symbol}", ("asset_symbol", *itr_for_one_asset_symbol));
				}
			}
		}

	}

	return void_result();
}

void_result token_configurator::apply(const module_cfg_operation& o)
{
	return void_result();
}


/**********************************************************************
敏感词库不使用大写字母
example for "WORD" module_cfg_object

{
  "id": "1.20.2",
  "module_name": "WORD",
  "module_cfg": {
    "sensitive_words": ["btc","eth","eos"]
  },
  "pending_module_cfg": {},
  "last_update_time": "2018-2-22T15:00:00",
  "last_modifier": "1.2.6"
}

**********************************************************************/
void_result word_configurator::evaluate(const module_cfg_operation& o)
{
	FC_ASSERT(name == o.module_name, "module name not matched. Mine: ${myname}, passed: ${passed}",
			("myname", name)("passed", o.module_name));

	if ( o.op_type != module_cfg_op_delete)
	{
		const variant_object& value = o.cfg_value;

		//reserved_asset_symbol
		FC_ASSERT( value["sensitive_words"].is_array(), "sensitive_words should be an array, sensitive_words=${r}", ("r", value["sensitive_words"]));
		const fc::variants& sensitive_words = value["sensitive_words"].get_array();
		for (auto itr_for_one_sensitive_word = sensitive_words.begin(); itr_for_one_sensitive_word != sensitive_words.end(); ++itr_for_one_sensitive_word)
		{
			FC_ASSERT(itr_for_one_sensitive_word->is_string(), "word should be a string. word=${word}", ("word", *itr_for_one_sensitive_word));

			//敏感词库不使用大写字母
			string s = itr_for_one_sensitive_word->get_string();
			unsigned int len = s.size();
			char c = 0;
			for(unsigned int i = 0; i < len; i++) 
		    {  
		    	c = s[i];
		        FC_ASSERT( !(c >= 'A' && c <= 'Z'), "sensitive word can not contan [A-Z], word=${word}", ("word", s));
		    }
		}

	}

	return void_result();
}


void_result word_configurator::apply(const module_cfg_operation& o)
{
	vector<string> sensitive_words;
	variant_object value = o.cfg_value;
	const fc::variants& words = value["sensitive_words"].get_array();

	auto itr_for_one_sensitive_word = words.begin();
	if ( itr_for_one_sensitive_word != words.end() )
	{
		sensitive_words.clear();
		if (o.op_type != module_cfg_op_delete)
		{
			for (; itr_for_one_sensitive_word != words.end(); ++itr_for_one_sensitive_word)	
			{
				// handle one word
				sensitive_words.push_back(itr_for_one_sensitive_word->get_string());
			}
		}
	}

	//update db
    const auto& word_object_index = _db.get_index_type<word_index>().indices().get<by_id>();
    auto itr = word_object_index.begin();
    if (itr == word_object_index.end())
    {
    	//first time to insert a param, create word object first
    	_db.create<word_object>( [&]( word_object& obj ) {
		    obj.sensitive_words.assign(sensitive_words.begin(), sensitive_words.end());
		  });
		  ilog("word object created. size=${size}", ("size", sensitive_words.size()));
    }
    else
    {
    	// update or delete
   	    _db.modify( *itr, [&]( word_object& obj ){
	        obj.sensitive_words.assign(sensitive_words.begin(), sensitive_words.end());
	    });
	    ilog("word object update or delete. size=${size}", ("size", sensitive_words.size()));
    }

	return void_result();
}


/**********************************************************************
操作黑名单, 不使用大写字母
example for "BLACKLIST" module_cfg_object

{
  "id": "1.20.2",
  "module_name": "BLACKLIST",
  "module_cfg": {
    "name_list": ["andy","tom","tony"]
  },
  "pending_module_cfg": {},
  "last_update_time": "2018-2-22T15:00:00",
  "last_modifier": "1.2.6"
}

**********************************************************************/
void_result operation_blacklist_configurator::evaluate(const module_cfg_operation& o)
{
	FC_ASSERT(name == o.module_name, "module name not matched. Mine: ${myname}, passed: ${passed}",
			("myname", name)("passed", o.module_name));

	if ( o.op_type != module_cfg_op_delete)
	{
		const variant_object& value = o.cfg_value;

		//reserved_asset_symbol
		FC_ASSERT( value["name_list"].is_array(), "name_list should be an array, name_list=${r}", ("r", value["name_list"]));
		const fc::variants& names = value["name_list"].get_array();
		const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();

		for (auto itr_for_one_name = names.begin(); itr_for_one_name != names.end(); ++itr_for_one_name)
		{
			FC_ASSERT(itr_for_one_name->is_string(), "name should be a string. name=${word}", ("word", *itr_for_one_name));

			//操作黑名单不使用大写字母
			string s = itr_for_one_name->get_string();
			unsigned int len = s.size();
			char c = 0;
			for(unsigned int i = 0; i < len; i++) 
		    {  
		    	c = s[i];
		        FC_ASSERT( !(c >= 'A' && c <= 'Z'), "name can not contan [A-Z], name=${name}", ("name", s));
		    }

		    auto itr = accounts_by_name.find(s);
      		FC_ASSERT( itr != accounts_by_name.end(), "Can not find account for name=${name}", ("name", s));
		}
	}

	return void_result();
}


void_result operation_blacklist_configurator::apply(const module_cfg_operation& o)
{
	vector<string> name_list;
	variant_object value = o.cfg_value;
	const fc::variants& names = value["name_list"].get_array();

	auto itr_for_one_name = names.begin();
	if ( itr_for_one_name != names.end() )
	{
		name_list.clear();
		if (o.op_type != module_cfg_op_delete)
		{
			for (; itr_for_one_name != names.end(); ++itr_for_one_name)	
			{
				// handle one word
				name_list.push_back(itr_for_one_name->get_string());
			}
		}
	}

	//update db
    const auto& blacklist_object_index = _db.get_index_type<operation_blacklist_index>().indices().get<by_id>();
    auto itr = blacklist_object_index.begin();
    if (itr == blacklist_object_index.end())
    {
    	//first time to insert a param, create word object first
    	_db.create<operation_blacklist_object>( [&]( operation_blacklist_object& obj ) {
		    obj.name_list.assign(name_list.begin(), name_list.end());
		  });
		ilog("operation_blacklist_object created. size=${size}", ("size", name_list.size()));
    }
    else
    {
    	// update or delete
   	    _db.modify( *itr, [&]( operation_blacklist_object& obj ){
	        obj.name_list.assign(name_list.begin(), name_list.end());
	    });
	    ilog("operation_blacklist_object update or delete. size=${size}", ("size", name_list.size()));
    }

	return void_result();
}
} } // graphene::chain
