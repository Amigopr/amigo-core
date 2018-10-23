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
#include <graphene/chain/token_evaluator.hpp>
#include <graphene/chain/global.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/module_configurator.hpp>
#include <graphene/db/object_database.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>
#include "boost/regex.hpp"
#include <ctype.h>


namespace graphene { namespace chain {

class is_invoked_by_token_flag_setting
{
public:
	is_invoked_by_token_flag_setting(bool flag = true)
	{ 
		ilog("is_invoked_by_token_flag_setting() begin, is_invoked_by_token_flag=${flag}", ("flag", graphene::chain::is_invoked_by_token_flag));
		graphene::chain::is_invoked_by_token_flag = flag; 
		ilog("is_invoked_by_token_flag_setting() end, is_invoked_by_token_flag=${flag}", ("flag", graphene::chain::is_invoked_by_token_flag));
	}
	~is_invoked_by_token_flag_setting()
	{
		graphene::chain::is_invoked_by_token_flag = false;
		ilog("~is_invoked_by_token_flag_setting(), is_invoked_by_token_flag=${flag}", ("flag", graphene::chain::is_invoked_by_token_flag)); 
	}
	
};

token_change_profie get_token_profile(const variant_object& value, bool get_reserved_asset_name_and_symbol)
{
	token_change_profie cfg;

	//token_types
	FC_ASSERT( value.contains("token_types"), "get_token_profile() token_types is not configured");
	const variant_object& types = value["token_types"].get_object();

	for (auto itr_for_one_type = types.begin(); itr_for_one_type != types.end(); ++itr_for_one_type)
	{
		const string& type_en = itr_for_one_type->key();
		const string& type_cn = itr_for_one_type->value().get_string();
		cfg.token_types[type_en] = type_cn;
	}

	//token_subtypes
	//if( !value["token_subtypes"].is_null() )
	if( value.contains("token_subtypes") )
	{
		const variant_object& subtypes = value["token_subtypes"].get_object();

		for (auto itr_for_one_type = subtypes.begin(); itr_for_one_type != subtypes.end(); ++itr_for_one_type)
		{
			const string& type_en = itr_for_one_type->key();
			
			//检查key(即token_type是否存在)
			map<string, string> subtypes_for_one_type;

			const variant_object& subtypes = itr_for_one_type->value().get_object();
			for (auto itr_for_one_subtype = subtypes.begin(); itr_for_one_subtype != subtypes.end(); ++itr_for_one_subtype)
			{
				subtypes_for_one_type[itr_for_one_subtype->key()] = itr_for_one_subtype->value().get_string(); //subtypes_for_one_type[subtype_en] = subtype_cn
			}
			cfg.token_subtypes[type_en] = subtypes_for_one_type;
		}
	}

	//asset_name_length
	FC_ASSERT( value.contains("name_length"), "get_token_profile() name_length is not configured");
	const variant_object& subvalue_5 = value["name_length"].get_object();
	cfg.name_length.min = subvalue_5["min"].as_uint64();
	cfg.name_length.max = subvalue_5["max"].as_uint64();

	//asset_symbol_length
	FC_ASSERT( value.contains("symbol_length"), "get_token_profile() symbol_length is not configured");
	const variant_object& subvalue_6 = value["symbol_length"].get_object();
	cfg.symbol_length.min = subvalue_6["min"].as_uint64();
	cfg.symbol_length.max = subvalue_6["max"].as_uint64();

	//token_brief_length
	FC_ASSERT( value.contains("brief_length"), "get_token_profile() brief_length is not configured");
	const variant_object& subvalue_7 = value["brief_length"].get_object();
	cfg.brief_length.min = subvalue_7["min"].as_uint64();
	cfg.brief_length.max = subvalue_7["max"].as_uint64();

	//token_description_length
	FC_ASSERT( value.contains("description_length"), "get_token_profile() description_length is not configured");
	const variant_object& subvalue_8 = value["description_length"].get_object();
	cfg.description_length.min = subvalue_8["min"].as_uint64();
	cfg.description_length.max = subvalue_8["max"].as_uint64();

	//max_days_between_create_and_phase1_begin
	FC_ASSERT( value.contains("max_days_between_create_and_phase1_begin"), "get_token_profile() max_days_between_create_and_phase1_begin is not configured");
	cfg.max_days_between_create_and_phase1_begin = value["max_days_between_create_and_phase1_begin"].as_uint64();

	//collect_days
	FC_ASSERT( value.contains("collect_days"), "get_token_profile() collect_days is not configured");
	const variant_object& subvalue_1 = value["collect_days"].get_object();
	cfg.collect_days.min = subvalue_1["min"].as_uint64();
	cfg.collect_days.max = subvalue_1["max"].as_uint64();

	//whitelist_max_size
	FC_ASSERT( value.contains("whitelist_max_size"), "get_token_profile() whitelist_max_size is not configured");
	cfg.whitelist_max_size = value["whitelist_max_size"].as_uint64();

	//guaranty_core_asset_months
	FC_ASSERT( value.contains("guaranty_core_asset_months"), "get_token_profile() guaranty_core_asset_months is not configured");
	const variant_object& subvalue_2 = value["guaranty_core_asset_months"].get_object();
	cfg.guaranty_core_asset_months.min = subvalue_2["min"].as_uint64();
	cfg.guaranty_core_asset_months.max = subvalue_2["max"].as_uint64();

	//each_buy_core_asset_range
	FC_ASSERT( value.contains("each_buy_core_asset_range"), "get_token_profile() each_buy_core_asset_range is not configured");
	const variant_object& subvalue_3 = value["each_buy_core_asset_range"].get_object();
	cfg.each_buy_core_asset_range.min = asset(std::stoll(subvalue_3["min"].get_string()), asset_id_type());
	cfg.each_buy_core_asset_range.max = asset(std::stoll(subvalue_3["max"].get_string()), asset_id_type());

	//issuer_reserved_asset_frozen_months
	FC_ASSERT( value.contains("issuer_reserved_asset_frozen_months"), "get_token_profile() issuer_reserved_asset_frozen_months is not configured");
	const variant_object& subvalue_4 = value["issuer_reserved_asset_frozen_months"].get_object();
	cfg.issuer_reserved_asset_frozen_months.min = subvalue_4["min"].as_uint64();
	cfg.issuer_reserved_asset_frozen_months.max = subvalue_4["max"].as_uint64();

	//buy_max_times
	FC_ASSERT( value.contains("buy_max_times"), "get_token_profile() buy_max_times is not configured");
	cfg.buy_max_times = value["buy_max_times"].as_uint64();

	if(!get_reserved_asset_name_and_symbol)
		return cfg;

	//reserved_asset_names
	FC_ASSERT( value.contains("reserved_asset_names"), "get_token_profile() reserved_asset_names is not configured");
	const fc::variants& reserved_asset_names = value["reserved_asset_names"].get_array();

	for (auto itr = reserved_asset_names.begin(); itr != reserved_asset_names.end(); ++itr)
	{
		FC_ASSERT(itr->is_string(), "asset_name should be a string. asset_name=${asset_name}", ("asset_name", *itr));
		string upper_case_asset_name;
		std::transform(itr->get_string().begin(), itr->get_string().end(), std::back_inserter(upper_case_asset_name), (int (*)(int))toupper);
		cfg.reserved_asset_names.push_back(upper_case_asset_name);
	}

	//reserved_asset_symbols
	FC_ASSERT( value.contains("reserved_asset_symbols"), "get_token_profile() reserved_asset_symbols is not configured");
	const fc::variants& reserved_asset_symbols = value["reserved_asset_symbols"].get_array();

	for (auto itr = reserved_asset_symbols.begin(); itr != reserved_asset_symbols.end(); ++itr)
	{
		FC_ASSERT(itr->is_string(), "asset_symbol should be a string. asset_symbol=${asset_symbol}", ("asset_symbol", *itr));
		cfg.reserved_asset_symbols.push_back(itr->get_string());
	}

	return cfg;
}

/****************************************************************************************
errno
10201001：发行者账号非法
10201002：发行者核心资产不足
10201003：用户资产名称/众筹项目名称太短
10201004：用户资产名称/众筹项目名称太长
10201005：用户资产缩写/项目简称太短
10201006：用户资产缩写/项目简称太长
10201007：用户资产缩写含有非法字符
10201008：logo链接太长
10201009：用户资产总(最大)发行量要大于等于1个通证
10201010：用户资产计划募集数量不能大于用户资产总(最大)发行量
10201011：认购阶段1不存在
10201012：认购阶段2不存在
10201013：认购阶段1结束时间要晚于开始时间
10201014：认购阶段2结束时间要晚于开始时间
10201015：认购阶段2开始时间不能早于认购阶段1结束时间
10201016：认购阶段1每份认购的核心资产数量要大于0
10201017：认购阶段1每份认购的用户资产数量要大于0
10201018：认购阶段2每份认购的核心资产数量要大于0
10201019：认购阶段2每份认购的用户资产数量要大于0
10201020：认购阶段1支付的核心资产应为AGC
10201021：认购阶段1和认购阶段2支付的核心资产应相同
10201022：认购阶段2的认购不应该比和认购阶段1优惠
10201023：发行人抵押核心资产的时长非法
10201024：行人预留的用户资产(未募集的用户资产部分)的解冻时长非法
10201025：用户资产募集白名单里有非注册用户
10201026：通证(众筹项目)简介长度超出限制
10201027：发行人自定义的通证(众筹项目)属性个数太多
10201028：用户资产计划募集数量必须大于认购阶段1和认购阶段2的每份认购的用户资产数量
10201029：用户资产计划募集数量必须大于等于0
10201030：用户资产计划募集数量等于最大发行量时，发行人预留的用户资产冻结时间只能为0
10201031：认购阶段1认购的用户资产缩写应为创建通证时的用户资产缩写
10201032：认购阶段2认购的用户资产缩写应和认购阶段1相同
10201033：发行人核心资产id只能为1.3.0
10201034：通证类型非法
10201035：通证配置中没有配置相应的子类型
10201036：通证子类型非法
10201037：用户资产名字只能包含字母
10201038：用户资产名字已经存在
10201039：用户资产缩写已经存在
10201040：需要募集时，募集白名单设置用户数量超出上限
10201041：需要募集时，用户资产募集成功的最少比例的数值应该在1-100
10201042：募集认购期不能短于3天
10201043：募集认购期不能超过90天
10201044：抵押的核心资产的资产id不对
10201045：用户资产总(最大)发行量超出上限(9000000000000000000)
10201046：用户资产名称包含有敏感词
10201047：用户资产缩写包含有敏感词
10201048：项目简介包含有敏感词
10201049：认购阶段1开始时间不能早于当前时间
10201050：认购阶段1开始时间不能迟当前时间的90天后
10201051：通证(众筹项目)描述长度不能超出限制
10201052：抵押核心资产的数量最多为1亿AGC
10201053：通证名称为系统保留的通证名称，不能用
10201054：通证缩写为系统保留的通证缩写，不能用
10201055：抵押核心资产数量为0时，抵押核心资产的时间只能为0
10201056：需要募集时，不能配置计划用于募集的通证数量为0
//10201057：当不需要募集而且只发行部分数量的通证时，发行数量必须小于总发行量
10201058：当不需要募集时，不需要抵押核心资产
10201059：通证(众筹项目)简介太短
10201060：通证(众筹项目)描述含有非法链接
10201061：创建通证时，参数配置错误
//10201062：发行人预留的通证(用户资产)数量大于0时，冻结时间周期数必须大于或等于1
10201063：认购阶段1，兑换比例中的AGC个数必须等于1
10201064：认购阶段2，兑换比例中的AGC个数必须等于1
10201065：字段内容长度超出限制
10201066：logo_url字段不符合https url规范
10201067：缺少poster_url字段
10201068：poster_url字段不符合https url规范
****************************************************************************************/
void_result token_publish_evaluator::do_evaluate( const token_publish_operation& op )
{ try {
	// check parameter
	database& d = db();
	const module_cfg_object& m = d.get_module_cfg("TOKEN");
	const token_change_profie token_profile = get_token_profile(m.module_cfg);

//	const auto& chain_parameters = db().get_global_properties().parameters;
//	ilog("create token profile: fee=${fee}, ~${param}", ("fee", op.fee)("param", chain_parameters.token_profile));
	// check account exist
	FC_ASSERT(d.find_object(op.issuer), "errno=10201001, issuer is invalid");

	FC_ASSERT( op.template_parameter.guaranty_core_asset_amount.asset_id == asset_id_type(), "errno=10201044, wrong guaranty_core_asset_amount.id. id=${asset_id}, guaranty_core_asset_amount=${g}", 
          ("asset_id", op.template_parameter.guaranty_core_asset_amount.asset_id)("g",op.template_parameter.guaranty_core_asset_amount) );

	// check account balance AGC
	issuer_ = &op.issuer(d);
	auto issuer_balance = d.get_balance( *issuer_, d.get(asset_id_type()) );
	FC_ASSERT( issuer_balance >= op.fee + op.template_parameter.guaranty_core_asset_amount, 
			"errno=10201002, insufficient balance. issuer_balance=${issuer_balance}, fee=${fee}, guaranty_core_asset_amount=${g}", 
        	("issuer_balance",issuer_balance)("fee",op.fee)("g", op.template_parameter.guaranty_core_asset_amount) );

	// check template_parameter

	//asset_name
	unsigned int len = op.template_parameter.asset_name.size();
	FC_ASSERT( len >= token_profile.name_length.min, "errno=10201003, asset_name is too short. asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));
	FC_ASSERT( len <= token_profile.name_length.max, "errno=10201004, asset_name is too long. asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));
	
	char c = 0;
	for(unsigned int i = 0; i < len; i++) //只允许大小写字母
    {  
    	c = op.template_parameter.asset_name[i];
        FC_ASSERT( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'), "errno=10201037, asset_name can only contain letters");
    }

	string upper_case_asset_name;
	std::transform(op.template_parameter.asset_name.begin(), op.template_parameter.asset_name.end(), std::back_inserter(upper_case_asset_name), (int (*)(int))toupper);

	//检查是否是系统保留的
	for (auto temp_itr = token_profile.reserved_asset_names.begin(); temp_itr != token_profile.reserved_asset_names.end(); ++temp_itr)
	{
		FC_ASSERT(upper_case_asset_name != *temp_itr, "errno=10201053, asset_name: ${asset_name} is reserved by system", ("asset_name", op.template_parameter.asset_name));
	}
	for (auto temp_itr = token_profile.reserved_asset_symbols.begin(); temp_itr != token_profile.reserved_asset_symbols.end(); ++temp_itr)
	{
		FC_ASSERT(upper_case_asset_name != *temp_itr, "errno=10201053, asset_name: ${asset_name} is reserved by system", ("asset_name", op.template_parameter.asset_name));
	}

	const auto& idx = d.get_index_type<token_index>().indices().get<by_upper_case_asset_name>();
	auto itr = idx.find(upper_case_asset_name);//如果这里为字符串null的话会导致程序崩溃	
	FC_ASSERT( itr == idx.end(), "errno=10201038, asset_name already exists, asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));

	//check if asset_name has existed within asset_symbol
	auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
    auto asset_symbol_itr = asset_indx.find( upper_case_asset_name );
    FC_ASSERT( asset_symbol_itr == asset_indx.end(), "errno=10201038, asset_name already exists, asset_name=${asset_name}", ("asset_name", op.template_parameter.asset_name));

    //敏感词检查
    string lower_case_asset_name;
	std::transform(op.template_parameter.asset_name.begin(), op.template_parameter.asset_name.end(), std::back_inserter(lower_case_asset_name), (int (*)(int))tolower);
	string result = word_contain_sensitive_word(lower_case_asset_name, d);
	FC_ASSERT( result == "", "errno=10201046, asset_name contains sensitive_word: ${result}", ("result", result));

	//asset_symbol
	len = op.template_parameter.asset_symbol.size();
	FC_ASSERT( len >= token_profile.symbol_length.min, "errno=10201005, asset_symbol is too short");
	FC_ASSERT( len <= token_profile.symbol_length.max, "errno=10201006, asset_symbol is too long");

	c = 0;
	for(unsigned int i = 0; i < len; i++)  
    {  
    	c = op.template_parameter.asset_symbol[i];
        //FC_ASSERT( (c >= 'A' && c <= 'Z') || c == '.', "errno=10201007, asset symbol can only contains characters: 'A'-'Z' and '.'");
        FC_ASSERT( c >= 'A' && c <= 'Z', "errno=10201007, asset symbol can only contains characters: 'A'-'Z'");
    }

    //检查是否是系统保留的
    for (auto temp_itr = token_profile.reserved_asset_names.begin(); temp_itr != token_profile.reserved_asset_names.end(); ++temp_itr)
	{
		FC_ASSERT(op.template_parameter.asset_symbol != *temp_itr, "errno=1020154, asset_symbol: ${asset_symbol} is reserved by system", ("asset_symbol", op.template_parameter.asset_symbol));
	}
	for (auto temp_itr = token_profile.reserved_asset_symbols.begin(); temp_itr != token_profile.reserved_asset_symbols.end(); ++temp_itr)
	{
		FC_ASSERT(op.template_parameter.asset_symbol != *temp_itr, "errno=1020154, asset_symbol: ${asset_symbol} is reserved by system", ("asset_symbol", op.template_parameter.asset_symbol));
	}
    

	//查找用户资产id
	itr = idx.find(op.template_parameter.asset_symbol);//如果这里为字符串null的话会导致程序崩溃	
	FC_ASSERT( itr == idx.end(), "errno=10201039, asset_symbol already exists, asset_symbol=${asset_symbol}", ("asset_symbol", op.template_parameter.asset_symbol));

	//check if asset_symbol has existed within asset_name
    asset_symbol_itr = asset_indx.find( op.template_parameter.asset_symbol );
    FC_ASSERT( asset_symbol_itr == asset_indx.end(), "errno=10201039, asset_symbol already exists, asset_symbol=${asset_symbol}", ("asset_symbol", op.template_parameter.asset_symbol));

	//敏感词检查
    string lower_case_asset_symbol;
	std::transform(op.template_parameter.asset_symbol.begin(), op.template_parameter.asset_symbol.end(), std::back_inserter(lower_case_asset_symbol), (int (*)(int))tolower);
	result = word_contain_sensitive_word(lower_case_asset_symbol, d);
	FC_ASSERT( result == "", "errno=10201047, asset_symbol contains sensitive_word: ${result}", ("result", result));


	//check logo_url
	FC_ASSERT( op.template_parameter.logo_url.size() <= 1024, "errno=10201008, logo_url is too long");
	//检查logo_url字段是否为https开头，并符合规范
	boost::regex https_reg("(https?)://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]");
	bool if_match = boost::regex_match(op.template_parameter.logo_url, https_reg);
	FC_ASSERT( if_match, "errno=10201066, logo_url not https url:${url}", ("url", op.template_parameter.logo_url));

	
	//check max_supply
	FC_ASSERT( std::stoll(op.template_parameter.max_supply) >= 100000000, "errno=10201009, max supply should be equal or larger than 100000000, max_supply=${max_supply}", ("max_supply", op.template_parameter.max_supply));
	FC_ASSERT( std::stoll(op.template_parameter.max_supply) <= 9000000000000000000, "errno=10201045, max supply is over limit(9000000000000000000), max_supply=${max_supply}", ("max_supply", op.template_parameter.max_supply));


	//check plan_buy_total
	FC_ASSERT( std::stoll(op.template_parameter.plan_buy_total) >= 0, "errno=10201029, Plan to collect total can not be less than 0");
	FC_ASSERT( std::stoll(op.template_parameter.plan_buy_total) <= std::stoll(op.template_parameter.max_supply), "errno=10201010, Plan to collect total can not be larger than max supply");

	if(std::stoll(op.template_parameter.plan_buy_total) == std::stoll(op.template_parameter.max_supply))
	{
		FC_ASSERT( op.template_parameter.issuer_reserved_asset_frozen_months == 0, 
			"errno=10201030, when plan_buy_total is equal to max_supply, issuer_reserved_asset_frozen_months must be 0");
	}

	//发行人预留的通证(用户资产)数量大于0时
	if(std::stoll(op.template_parameter.plan_buy_total) > 0 && std::stoll(op.template_parameter.plan_buy_total) < std::stoll(op.template_parameter.max_supply))
	{
		//check issuer_reserved_asset_frozen_months
		FC_ASSERT( op.template_parameter.issuer_reserved_asset_frozen_months >= token_profile.issuer_reserved_asset_frozen_months.min && 
					op.template_parameter.issuer_reserved_asset_frozen_months <= token_profile.issuer_reserved_asset_frozen_months.max,  
					"errno=10201024, issuer_reserved_asset_frozen_months(${i}) is invalid. valid range:[${min}, ${max}]", 
					("i", op.template_parameter.issuer_reserved_asset_frozen_months)("min", token_profile.issuer_reserved_asset_frozen_months.min)
					("max", token_profile.issuer_reserved_asset_frozen_months.max));
	}

	//check type
	//const std::map<string, string>::iterator it;
	auto it = token_profile.token_types.find(op.template_parameter.type);
	FC_ASSERT( it != token_profile.token_types.end(), "errno=10201034, the token type does not exist. type=${type}", ("type", op.template_parameter.type));

	//check subtype
	if( token_profile.token_subtypes.size() > 0 && op.template_parameter.subtype != "" && op.template_parameter.type != "")
	{
		auto it2 = token_profile.token_subtypes.find(op.template_parameter.type);
		//FC_ASSERT( it2 != token_profile.token_subtypes.end(), "errno=10201035, the subtypes corresponding to the token type does not exist. type=${type}", ("type", op.template_parameter.type));
		if(it2 != token_profile.token_subtypes.end())// type exists
		{
			//const std::map<string, string>::iterator it3;
			auto it3 = it2->second.find(op.template_parameter.subtype);
			FC_ASSERT( it3 != it2->second.end(), "errno=10201036, subtype does not exist. type=${type}, subtype=${subtype}", ("type", op.template_parameter.type)("subtype", op.template_parameter.subtype));
		}
	}

	FC_ASSERT( !op.template_parameter.need_raising, "needing raising must be false by present");//目前通证暂不开放募集功能
	FC_ASSERT( !(op.template_parameter.need_raising && std::stoll(op.template_parameter.plan_buy_total) == 0) , "errno=10201056, when needing raising, plan_buy_total can not be 0");

	//check  buy_phases
	if( op.template_parameter.need_raising && std::stoll(op.template_parameter.plan_buy_total) > 0 )//需要募集
	{
	    // 检查各个认购阶段的设置
		//check phase1 and phase2
		auto itr1 = op.template_parameter.buy_phases.find("1"); //find phase1
		FC_ASSERT( itr1 != op.template_parameter.buy_phases.end(), "errno=10201011, no phase1 when creating token");
		auto itr2 = op.template_parameter.buy_phases.find("2"); //find phase2
		FC_ASSERT( itr2 != op.template_parameter.buy_phases.end(), "errno=10201012, no phase2 when creating token");

		FC_ASSERT( std::stoll(op.template_parameter.plan_buy_total) > itr1->second.quote_base_ratio.quote.amount && std::stoll(op.template_parameter.plan_buy_total) > itr2->second.quote_base_ratio.quote.amount, 
			"errno=10201028, Plan to buy total must be larger than quote amount of one buy in phase1 and phase2");

		FC_ASSERT( itr1->second.begin_time >= d.head_block_time(), "errno=10201049, phase1 begin time should be later than current time");
		FC_ASSERT( itr1->second.begin_time <= d.head_block_time() + 86400 * token_profile.max_days_between_create_and_phase1_begin, 
				"errno=10201050, phase1 begin time should be within than 90 days after current time");
		FC_ASSERT( itr1->second.end_time > itr1->second.begin_time, "errno=10201013, phase1 end time should be later than start time");
		FC_ASSERT( itr2->second.end_time > itr2->second.begin_time, "errno=10201014, phase2 end time should be later than start time");
		FC_ASSERT( itr2->second.begin_time >= itr1->second.end_time, "errno=10201015, phase2 start time should not be earlier than phase1 end time");

		// 项目募集开放的时长（募集认购期）最短3天，最长3个月（每个月按30天计算）, 1000000*3600*24微秒=86400000000微秒
		fc::microseconds diff = itr2->second.end_time - itr1->second.begin_time;
		share_type min_buy_duration = 86400000000 * token_profile.collect_days.min;
		share_type max_buy_duration = 86400000000 * token_profile.collect_days.max;
		FC_ASSERT( diff.count() >= min_buy_duration, "errno=10201042, the duration between phase1 and phase2 can not be less than ${min} days", ("min", token_profile.collect_days.min));
		FC_ASSERT( diff.count() <= max_buy_duration, "errno=10201043 the duration between phase1 and phase2 can not be more than ${max} days", ("max", token_profile.collect_days.max));
		
		FC_ASSERT( itr1->second.quote_base_ratio.base.amount > 0, "errno=10201016, phase1 base amount should be larger than 0");
		FC_ASSERT( itr1->second.quote_base_ratio.quote.amount > 0, "errno=10201017, phase1 quote amount should be larger than 0");
		FC_ASSERT( itr2->second.quote_base_ratio.base.amount > 0, "errno=10201018, phase2 base amount should be larger than 0");
		FC_ASSERT( itr2->second.quote_base_ratio.quote.amount > 0, "errno=10201019, phase1 quote amount should be larger than 0");

		//FC_ASSERT( itr1->second.quote_base_ratio.base.amount == GRAPHENE_BLOCKCHAIN_PRECISION, "errno=10201063, phase1 base amount should be equal to ${number}", ("number", GRAPHENE_BLOCKCHAIN_PRECISION));
		//FC_ASSERT( itr2->second.quote_base_ratio.base.amount == GRAPHENE_BLOCKCHAIN_PRECISION, "errno=10201064, phase2 base amount should be equal to ${number}", ("number", GRAPHENE_BLOCKCHAIN_PRECISION));

		FC_ASSERT( itr1->second.quote_base_ratio.base.symbol == "AGC", "errno=10201020, phase1 base symbol should be AGC");
		FC_ASSERT( itr2->second.quote_base_ratio.base.symbol == itr1->second.quote_base_ratio.base.symbol, "errno=10201021, phase1 base symbol should be the same as phase2");

		FC_ASSERT( itr1->second.quote_base_ratio.quote.symbol == op.template_parameter.asset_symbol, "errno=10201031, phase1 quote symbol should be ${asset_symbol}", ("asset_symbol", op.template_parameter.asset_symbol));
		FC_ASSERT( itr2->second.quote_base_ratio.quote.symbol == itr1->second.quote_base_ratio.quote.symbol, "errno=10201032, phase1 quote symbol should be the same as phase2");

		if( itr1->second.quote_base_ratio.base.amount >= itr1->second.quote_base_ratio.quote.amount)
		{
			auto phase1_ratio = itr1->second.quote_base_ratio.base.amount / itr1->second.quote_base_ratio.quote.amount;
			auto phase2_ratio = itr2->second.quote_base_ratio.base.amount / itr2->second.quote_base_ratio.quote.amount;
			FC_ASSERT( phase1_ratio <= phase2_ratio, "errno=10201022, phase1 buy price should not be more expensive than phase2");
		}
		else
		{
			auto phase1_ratio = itr1->second.quote_base_ratio.quote.amount / itr1->second.quote_base_ratio.base.amount;
			auto phase2_ratio = itr2->second.quote_base_ratio.quote.amount / itr2->second.quote_base_ratio.base.amount;
			FC_ASSERT( phase1_ratio >= phase2_ratio, "errno=10201022, phase1 buy price should not be more expensive than phase2");

		}

		FC_ASSERT( op.template_parameter.buy_succeed_min_percent >= 1 && op.template_parameter.buy_succeed_min_percent <= 100, 
				"errno=10201041, buy_succeed_min_percent should be between 1 and 100 when it needs crowdfunding");

		FC_ASSERT( op.template_parameter.guaranty_core_asset_amount.asset_id == asset_id_type(),  
					"errno=10201033, guaranty_core_asset_amount.id must be 1.3.0");

		FC_ASSERT( op.template_parameter.guaranty_core_asset_amount.amount <= 10000000000000000, 
				"errno=10201052, guaranty_core_asset_amount is 0-100000000 AGC");//1亿

		//check guaranty_core_asset_months
		if( op.template_parameter.guaranty_core_asset_amount.amount > 0 )
		{
			FC_ASSERT( op.template_parameter.guaranty_core_asset_months >= token_profile.guaranty_core_asset_months.min && 
					op.template_parameter.guaranty_core_asset_months <= token_profile.guaranty_core_asset_months.max,  
					"errno=10201023, guaranty_core_asset_months(${g}) is invalid. valid range:[${min}, ${max}]", 
					("g", op.template_parameter.guaranty_core_asset_months)("min", token_profile.guaranty_core_asset_months.min)("max", token_profile.guaranty_core_asset_months.max));
		}
		else
		{
			FC_ASSERT( op.template_parameter.guaranty_core_asset_months == 0, "errno=10201055, guaranty_core_asset_months must be 0 when guaranty_core_asset is 0");
		}

		//check whitelist
		FC_ASSERT( op.template_parameter.whitelist.size() <= token_profile.whitelist_max_size, "errno=10201040, whitelist size should exceed max. size=${size}, max=${max}", 
				("size", op.template_parameter.whitelist.size())("max", token_profile.whitelist_max_size));

		if( !op.template_parameter.whitelist.empty() )
		{
			for(vector<string>::const_iterator it = op.template_parameter.whitelist.begin(); it!=op.template_parameter.whitelist.end(); ++it)  
		    {  
		    	const auto& accounts_by_name = db().get_index_type<account_index>().indices().get<by_name>();
		    	auto it2 = accounts_by_name.find(*it);

	     		FC_ASSERT( !(it2 == accounts_by_name.end()), "errno=10201025, In whitelist the name: ${name} is not a registered user", ("name", *it));
		    }
		}
	}//if(std::stoll(op.template_parameter.plan_buy_total) > 0)
	else if( !op.template_parameter.need_raising && std::stoll(op.template_parameter.plan_buy_total) >= 0 &&
				std::stoll(op.template_parameter.plan_buy_total) <= std::stoll(op.template_parameter.max_supply) )//不需要募集
	{
/*		FC_ASSERT( std::stoll(op.template_parameter.plan_buy_total) <= std::stoll(op.template_parameter.max_supply), 
			"errno=10201057, plan_buy_total should be less than max_supply when it does not need rasing and plan_buy_total > 0. plan_buy_total= ${plan_buy_total}, max_supply=${max_supply}", 
			("plan_buy_total", op.template_parameter.plan_buy_total)("max_supply", op.template_parameter.max_supply));
*/
//兼容测试数据
		if( d.head_block_time() > fc::time_point_sec( 1522495080 ) ) //2018/3/31 19:18:0
//兼容测试数据
			FC_ASSERT( op.template_parameter.guaranty_core_asset_amount.amount == 0 && op.template_parameter.guaranty_core_asset_months == 0, 
				"errno=10201058, guaranty_core_asset_amount.amount and guaranty_core_asset_months should be 0 when it does not need rasing and plan_buy_total > 0");
	}
	else
	{
		FC_ASSERT( false, "errno=10201061, wrong parameters are configured, max_supply=${max_supply}, need_raising=${need_raising}, plan_buy_total=${plan_buy_total}", 
			("max_supply", op.template_parameter.max_supply)("need_raising", op.template_parameter.need_raising)("plan_buy_total", op.template_parameter.plan_buy_total));
	}

	//check brief
	FC_ASSERT( op.template_parameter.brief.size() >= token_profile.brief_length.min, "errno=10201059, the length of token brief should contain at least ${min} characters",
			("min", token_profile.brief_length.min));

	FC_ASSERT( op.template_parameter.brief.size() <= token_profile.brief_length.max, "errno=10201026, the length of token brief can not be longer than ${max}, length=${length}",
			("max", token_profile.brief_length.max)("length", op.template_parameter.brief.size()));
	
	string lower_case_brief;
	std::transform(op.template_parameter.brief.begin(), op.template_parameter.brief.end(), std::back_inserter(lower_case_brief), (int (*)(int))tolower);
	result = string_contain_sensitive_word(lower_case_brief, d);
	FC_ASSERT( result == "", "errno=10201048, brief contains sensitive_word: ${result}", ("result", result));

	//check description
	FC_ASSERT( op.template_parameter.description.size() <= token_profile.description_length.max, "errno=10201051, the length of token description can not be longer than ${max}",
			("max", token_profile.description_length.max));

	//check if description contains invalid website link
	boost::regex reg(".*<a.*href[ \f\n\r\t\v]*=.*>.*");
	bool match = boost::regex_match(op.template_parameter.description, reg);
	FC_ASSERT( !match, "errno=10201060, token description contains invalid website link");

	//check customized_attributes
	FC_ASSERT( op.template_parameter.customized_attributes.size() <= 10, "errno=10201027, the number of customized attributes can not be larger than 10");

    //海报URL为必须字段
    bool if_exist_poster_url = false;
	if(op.exts)
	{
		for(auto temp_itr = op.exts->begin(); temp_itr != op.exts->end(); ++temp_itr)
		{
			FC_ASSERT( temp_itr->first.length() <= 64, "errno=10201065, the length of key field exceeds limit 64");
			FC_ASSERT( temp_itr->second.length() <= 51200, "errno=10201065, the length of value field exceeds limit 51200");

			if(temp_itr->first == "poster_url")
			{
			    bool if_match1 = boost::regex_match(temp_itr->second, https_reg);
	            FC_ASSERT( if_match1, "errno=10201068, poster_url not https url:${url}", ("url", temp_itr->second));
	
			    if_exist_poster_url = true;
			}
		}
	}

	FC_ASSERT( if_exist_poster_url, "errno=10201067, lack of poster_url");

	upper_case_asset_name_  = upper_case_asset_name;
	create_time_ 			= d.head_block_time();
	phase1_begin_time_		= op.template_parameter.buy_phases.find("1")->second.begin_time;
    phase1_end_time_    	= op.template_parameter.buy_phases.find("1")->second.end_time;
    phase2_begin_time_  	= op.template_parameter.buy_phases.find("2")->second.begin_time;
    phase2_end_time_    	= op.template_parameter.buy_phases.find("2")->second.end_time;
    settle_time_        	= phase2_end_time_;

	return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void token_publish_evaluator::pay_fee()
{
	// input to fee pool
	deferred_fee_ = core_fee_paid;
	ilog("token publish, fee=${fee}", ("fee", deferred_fee_));

}


object_id_type token_publish_evaluator::do_apply( const token_publish_operation& op )
{ try {
	ilog("token publish, asset=${asset_symbol}", ("asset_symbol", op.template_parameter.asset_symbol));	
	is_invoked_by_token_flag_setting auto_lock;

	database& d = db();
	transaction_evaluation_state context(&d);

	//创建用户资产
	asset_create_operation c;
	c.issuer = op.issuer;
	c.symbol = op.template_parameter.asset_symbol;
	c.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;

	//common_options
	c.common_options.max_supply = std::stoll(op.template_parameter.max_supply);
	c.common_options.market_fee_percent = 0;
	c.common_options.issuer_permissions = 0x10; //disable force settling. see enum asset_issuer_permission_flags in file types.hpp
	c.common_options.flags = 0;
	//description的格式:"{\"main\":\"Token Brief\",\"short_name\":\"AssetName\"}"
	char s[2048] = {0};
	sprintf(s, "{\"main\":\"%s\",\"short_name\":\"%s\"}", op.template_parameter.brief.c_str(), op.template_parameter.asset_name.c_str());
	c.common_options.description = s;
	c.is_prediction_market = false;	
	d.current_fee_schedule().calculate_fee(c);

	// inner undo session for not interupt block handle
	auto session = d._undo_db.start_undo_session(true);
	d.apply_operation(context, c);
	session.merge();
	ilog("token_publish_evaluator::do_apply, asset ${asset_symbol} create ok", ("asset_symbol", op.template_parameter.asset_symbol));

	//查找新创建用户资产id是否存在
	auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
    auto asset_symbol_itr = asset_indx.find( op.template_parameter.asset_symbol );
    FC_ASSERT( asset_symbol_itr != asset_indx.end() );
    ilog("token_publish_evaluator::do_apply, find asset ${asset_symbol} ok", ("asset_symbol", op.template_parameter.asset_symbol));

	//发行用户资产
	asset_issue_operation i;
	i.issuer = op.issuer;
	i.issue_to_account = op.issuer;
	i.asset_to_issue = asset(std::stoll(op.template_parameter.max_supply), asset_symbol_itr->id); 
	//i.fee = asset(0, asset_id_type());
	d.current_fee_schedule().calculate_fee(i);

	session = d._undo_db.start_undo_session(true);
	d.apply_operation(context, i);
	session.merge();
	ilog("token_publish_evaluator::do_apply, asset ${asset_symbol} issue ok", ("asset_symbol", op.template_parameter.asset_symbol));


	//创建众筹项目

    auto next_token_id = db().get_index_type<token_index>().get_next_id();

	const token_statistics_object& dyn_token =
      db().create<token_statistics_object>( [&]( token_statistics_object& a ) {
      	 a.token_id				= next_token_id;
         a.buyer_number 			= 0;
         a.actual_buy_total 		= 0;
         a.actual_buy_percentage  	= 0;
         a.actual_not_buy_total     = std::stoll(op.template_parameter.plan_buy_total);
         a.has_returned_guaranty_core_asset     = asset(0, asset_id_type());
         a.has_returned_issuer_reserved_asset   = asset(0, asset_symbol_itr->id);
    });

	// buy pay: db().adjust_balance(o.issuer, -o.issuer_buy());

	const auto& new_token_object = db().create<token_object>( [&]( token_object& obj ){
		
		obj.issuer 		          			= op.issuer;
		obj.upper_case_asset_name 			= upper_case_asset_name_;
		obj.user_issued_asset_id 			= asset_symbol_itr->id;// o.template_parameter.asset_symbol;
		obj.buy_succeed_min_amount			= std::stoll(op.template_parameter.plan_buy_total) * op.template_parameter.buy_succeed_min_percent / GRAPHENE_1_PERCENT;
		obj.deferred_fee 	        		= deferred_fee_;
		
		obj.status_expires.create_time 		= create_time_;
		obj.status_expires.phase1_begin 	= phase1_begin_time_;
		obj.status_expires.phase1_end 		= phase1_end_time_;
		obj.status_expires.phase2_begin 	= phase2_begin_time_;
		obj.status_expires.phase2_end 		= phase2_end_time_;
		obj.status_expires.settle_time 		= settle_time_; //如果认购提前结束, 实际结算时间也会提前，实际结算时间在database::apply_token_buy()再更新

		obj.exts 							= op.exts;//

		if(op.template_parameter.need_raising && std::stoll(op.template_parameter.plan_buy_total) > 0) //需要募集
		{
			obj.template_parameter				= op.template_parameter;
			obj.issuer_reserved_asset_total		= std::stoll(op.template_parameter.max_supply) - std::stoll(op.template_parameter.plan_buy_total);
			obj.status 							= token_object::token_status::create_status;

			if(obj.template_parameter.guaranty_core_asset_amount.amount > 0) //发行人抵押的核心资产(AGC) > 0
			{
				d.adjust_balance(op.issuer, -obj.template_parameter.guaranty_core_asset_amount); //先减去用户资产发行人发行的用户资产的计划众筹部分
			}

			//发行人抵押的核心资产
			if(op.template_parameter.guaranty_core_asset_months > 0)
				obj.each_period_return_guaranty_core_asset   = op.template_parameter.guaranty_core_asset_amount.amount / op.template_parameter.guaranty_core_asset_months;//发行人抵押的核心资产(AGC)是分期返还
				//obj.each_period_return_guaranty_core_asset   = op.template_parameter.guaranty_core_asset_amount.amount; //发行人抵押的核心资产(AGC)是一次性返还
			else
				obj.each_period_return_guaranty_core_asset   = 0;

			//下面的需要等待募集结束(结算)时才能最终确定，返还开始计算时间先定义为众筹第2阶段结束时间，如果募集提前结束，再更新
			if( op.template_parameter.guaranty_core_asset_amount.amount > 0 && op.template_parameter.guaranty_core_asset_months > 0)
			{
				//发行人抵押的核心资产(AGC)是逐月返还
				obj.status_expires.next_return_guaranty_core_asset_time = phase2_end_time_ + SECONDS_OF_ONE_MONTH; //1个月按30天算，1 month = 2592000 seconds
				obj.status_expires.return_guaranty_core_asset_end = phase2_end_time_ + op.template_parameter.guaranty_core_asset_months * SECONDS_OF_ONE_MONTH;
			}
			else
			{
				obj.status_expires.next_return_guaranty_core_asset_time = phase2_end_time_;
				obj.status_expires.return_guaranty_core_asset_end = phase2_end_time_;
			}
			
			if(obj.issuer_reserved_asset_total > 0 && op.template_parameter.issuer_reserved_asset_frozen_months > 0)
				obj.status_expires.next_return_issuer_reserved_asset_time = phase2_end_time_ + SECONDS_OF_ONE_MONTH; //1个月按30天算，1 month = 2592000 seconds
			else
				obj.status_expires.next_return_issuer_reserved_asset_time = phase2_end_time_;

			obj.status_expires.return_issuer_reserved_asset_end = phase2_end_time_ + op.template_parameter.issuer_reserved_asset_frozen_months * SECONDS_OF_ONE_MONTH;
			//obj.status_expires.return_asset_end = max(obj.status_expires.return_guaranty_core_asset_end, obj.status_expires.return_issuer_reserved_asset_end);
			obj.status_expires.return_asset_end = 
				obj.status_expires.return_guaranty_core_asset_end >= obj.status_expires.return_issuer_reserved_asset_end ? obj.status_expires.return_guaranty_core_asset_end : obj.status_expires.return_issuer_reserved_asset_end;

//			if(obj.issuer_reserved_asset_total > 0 && op.template_parameter.issuer_reserved_asset_frozen_months == 0) //发行人预留的用户资产(未募集的用户资产部分) >0 && 发行人预留的用户资产(未募集的用户资产部分)解冻时长=0个月
//			{
//				d.adjust_balance(op.issuer, -asset(std::stoll(op.template_parameter.plan_buy_total), asset_symbol_itr->id)); //先减去用户资产发行人发行的用户资产的计划众筹部分
//			}
//			else//发行人预留的用户资产=0 || (发行人预留的用户资产>0 && 发行人预留的用户资产(未募集的用户资产部分)解冻时长 >= 1个月)
//			{
				d.adjust_balance(op.issuer, -asset(std::stoll(op.template_parameter.max_supply), asset_symbol_itr->id));//先减去用户资产发行人发行的全部用户资产, 发行人预留的用户资产统一在募集结束后才开始解冻	
//			}

		}
		else if(!op.template_parameter.need_raising && 
				(std::stoll(op.template_parameter.plan_buy_total) == 0 || std::stoll(op.template_parameter.plan_buy_total) == std::stoll(op.template_parameter.max_supply)))//表示直接创建通证，不需要募集，所有的通证一次性直接给发行人
		{
			obj.template_parameter										= op.template_parameter;
			obj.template_parameter.plan_buy_total						= op.template_parameter.max_supply;//这里填最大发行量，表示创建通证时,所有的通证一次性直接给发行人
			obj.template_parameter.buy_succeed_min_percent				= 0;
			obj.template_parameter.not_buy_asset_handle 				= 0;
			obj.template_parameter.guaranty_core_asset_amount			= asset(0, asset_id_type());
			obj.template_parameter.guaranty_core_asset_months 			= 0;
			obj.template_parameter.issuer_reserved_asset_frozen_months 	= 0;
			obj.issuer_reserved_asset_total								= 0;
			obj.status 													= token_object::token_status::close_status;
		}
		else if( !op.template_parameter.need_raising && std::stoll(op.template_parameter.plan_buy_total) > 0 && 
				std::stoll(op.template_parameter.plan_buy_total) < std::stoll(op.template_parameter.max_supply) )//不需要募集，直接给发行人plan_buy_total数量的通证(可视为在其它平台上进行募集)，而且也不抵押，通证剩余部分分期发放(允许期数为0)
		{
			obj.template_parameter										= op.template_parameter;
			obj.template_parameter.guaranty_core_asset_amount			= asset(0, asset_id_type());
			obj.template_parameter.guaranty_core_asset_months 			= 0;
			obj.issuer_reserved_asset_total								= std::stoll(op.template_parameter.max_supply) - std::stoll(op.template_parameter.plan_buy_total);
			obj.status 													= token_object::token_status::settle_status;

			//下面的需要等待募集结束(结算)时才能最终确定，返还开始计算时间先定义为众筹第2阶段结束时间，如果募集提前结束，再更新	
			if(obj.issuer_reserved_asset_total > 0 && op.template_parameter.issuer_reserved_asset_frozen_months > 0)
				obj.status_expires.next_return_issuer_reserved_asset_time = d.head_block_time() + SECONDS_OF_ONE_MONTH; //1个月按30天算，1 month = 2592000 seconds
			else
				obj.status_expires.next_return_issuer_reserved_asset_time = d.head_block_time();

			obj.status_expires.return_issuer_reserved_asset_end = d.head_block_time() + op.template_parameter.issuer_reserved_asset_frozen_months * SECONDS_OF_ONE_MONTH; //允许op.template_parameter.issuer_reserved_asset_frozen_months为0
			obj.status_expires.return_asset_end = obj.status_expires.return_issuer_reserved_asset_end;

			d.adjust_balance(op.issuer, -asset(obj.issuer_reserved_asset_total, asset_symbol_itr->id));

		}
		else
		{
			FC_ASSERT( false, "token_publish_evaluator::do_apply() wrong handle, max_supply=${max_supply}, need_raising=${need_raising}, plan_buy_total=${plan_buy_total}", 
				("max_supply", op.template_parameter.max_supply)("need_raising", op.template_parameter.need_raising)("plan_buy_total", op.template_parameter.plan_buy_total));
		}

		if(op.template_parameter.issuer_reserved_asset_frozen_months > 0 )
        	obj.each_period_return_issuer_reserved_asset = obj.issuer_reserved_asset_total / op.template_parameter.issuer_reserved_asset_frozen_months;
        else
        	obj.each_period_return_issuer_reserved_asset = 0;

        //抵押信用
		obj.guaranty_credit = obj.template_parameter.guaranty_core_asset_amount.amount / GRAPHENE_BLOCKCHAIN_PRECISION * op.template_parameter.guaranty_core_asset_months; //为避免溢出，去掉小数部分

		obj.statistics		                = dyn_token.id;
	});

    assert( new_token_object.id == next_token_id );

	ilog("create publish token, id=${id}, asset_symbol=${asset_symbol}, status=${status_expires}", 
			("id", new_token_object.id)("asset_symbol", new_token_object.template_parameter.asset_symbol)("status_expires", new_token_object.status_expires));	

	return new_token_object.id;

} FC_CAPTURE_AND_RETHROW( (op) ) }

/****************************************************************************************
errno
10202001：该通证(众筹项目)不能认购
10202002：认购份数要大于0
10202003：当前时间不在认购第1阶段内
10202004：没有配置认购第1阶段
10202005：当前时间不在认购第2阶段内
10202006：没有配置认购第2阶段
10202007：可以认购的通证数量余额不足
10202008：发行人核心资产不足
10202009：同一众筹项目一个用户可认购10次
10202010：认购者不在认购白名单内
10202011：认购数量已超出目前可认购的通证数量
****************************************************************************************/
void_result token_buy_evaluator::do_evaluate( const token_buy_operation& op )
{ try {
	ilog("token_buy_evaluator::do_evaluate, id = ${id}", ("id", op.token_id));
	// check parameter
	database& d = db();
	//const chain_parameters& chain_parameters = db().get_global_properties().parameters;
	const module_cfg_object& m = d.get_module_cfg("TOKEN");
	const token_change_profie token_profile = get_token_profile(m.module_cfg, false);

	// check account
	FC_ASSERT(d.find_object(op.buyer), "buyer is invalid");

	// check token_id
	FC_ASSERT(d.find_object(op.token_id));
	auto& index = d.get_index_type<token_index>().indices().get<by_id>();
	auto token_itr = index.find(op.token_id);
	FC_ASSERT( token_itr != index.end() );
	auto& token = *token_itr;
	auto& token_statistics = token.statistics(d);

	// check if it can buy
	FC_ASSERT(token.enable_buy(), "errno=10202001, buy is unavailable for this token ");
	FC_ASSERT( op.template_parameter.buy_quantity > 0, "errno=10202002, buy_quantity should be larger than 0. buy_quantity=${buy_quantity}", ("buy_quantity", op.template_parameter.buy_quantity));

	// check if the buyer is within whitelist
	if(!token.template_parameter.whitelist.empty())
	{
		bool isInWhiteList = false;
		for(vector<string>::const_iterator it = token.template_parameter.whitelist.begin(); it!=token.template_parameter.whitelist.end(); ++it)  
	    {  
	    	//account_id_type account_id = get_account(*it).get_id();

			const account_object* account = nullptr;
			string creator_name_or_id = *it;
			if (std::isdigit(creator_name_or_id[0]))
				account = d.find(fc::variant(creator_name_or_id).as<account_id_type>());
			else
			{
				const auto& idx = d.get_index_type<account_index>().indices().get<by_name>();
				auto itr = idx.find(creator_name_or_id);//如果这里为字符串null的话会导致程序崩溃
				if (itr != idx.end())
					account = &*itr;
			}
			if (account != nullptr && account->id == op.buyer)
			{
				isInWhiteList = true;
	    		break;
			}
	    }
	    
	    FC_ASSERT( isInWhiteList, "errno=10202010, The buyer is not within whitelist");
	}

	if(op.template_parameter.phase == token_buy_template::token_buy_phase::buy_phase1)
	{
		FC_ASSERT((token.phase1_begin_time() <= d.head_block_time() && token.phase1_end_time() >= d.head_block_time()), 
	            "errno=10102003, it is not within buy_phase1.  Buy is unavailable. phase1_begin_time=${b}, phase1_end_time=${e}, current=${h}", 
	            ("b", token.phase1_begin_time())("e", token.phase1_end_time())("h", d.head_block_time()));

		auto itr = token.template_parameter.buy_phases.find("1");
		FC_ASSERT( itr != token.template_parameter.buy_phases.end(), "errno=10102004, no phase1 for token. token id = ${id}", ("id", op.token_id));
	}

	if(op.template_parameter.phase == token_buy_template::token_buy_phase::buy_phase2)
	{
		FC_ASSERT((token.phase2_begin_time() <= d.head_block_time() && token.phase2_end_time() >= d.head_block_time()),
	            "errno=10102005, it is not within buy_phase2. Buy is unavailable. phase2_begin_time=${b}, phase2_end_time=${e}, current=${h}", 
	            ("b", token.phase2_begin_time())("e", token.phase2_end_time())("h", d.head_block_time()));

	    // check buy_amount
		auto itr = token.template_parameter.buy_phases.find("2");
		FC_ASSERT( itr != token.template_parameter.buy_phases.end(), "errno=10102006, no phase2 for token. token id = ${id}", ("id", op.token_id));
	}

	// check balance AGC
	share_type pay_amount = 0;
	share_type buy_amount = 0;

    if(op.template_parameter.phase == token_buy_template::token_buy_phase::buy_phase1)//认购阶段1
	{
		//base
		pay_amount = op.get_buy_quantity() * token.template_parameter.buy_phases.find("1")->second.quote_base_ratio.base.amount;
		//quote
		buy_amount = op.get_buy_quantity() * token.template_parameter.buy_phases.find("1")->second.quote_base_ratio.quote.amount;
	}
	else//认购阶段2
	{
		//base
		pay_amount = op.get_buy_quantity() * token.template_parameter.buy_phases.find("2")->second.quote_base_ratio.base.amount;
		//quote
		buy_amount = op.get_buy_quantity() * token.template_parameter.buy_phases.find("2")->second.quote_base_ratio.quote.amount;
	}

	char str_actual_buy_total[1024]={0};
	char str_actual_not_buy_total[1024]={0};
	char str_buy_amount[1024]={0};
	char str_plan_buy_total[1024]={0};

	sprintf(str_actual_buy_total, "%20.8f", ((double)token_statistics.actual_buy_total.value) / GRAPHENE_BLOCKCHAIN_PRECISION);
	sprintf(str_actual_not_buy_total, "%20.8f", ((double)token_statistics.actual_not_buy_total.value) / GRAPHENE_BLOCKCHAIN_PRECISION);
	sprintf(str_buy_amount, "%20.8f", ((double)buy_amount.value) / GRAPHENE_BLOCKCHAIN_PRECISION);
	sprintf(str_plan_buy_total, "%20.8f", (std::stod(token.template_parameter.plan_buy_total)) / GRAPHENE_BLOCKCHAIN_PRECISION);

	FC_ASSERT((token_statistics.actual_not_buy_total >= buy_amount),
			"errno=10202007, available buy is insufficient. fields={\"actual_not_buy_total\":${a}, \"buy_amount\":${b}}", 
			("a", str_actual_not_buy_total)("b", str_buy_amount));
	FC_ASSERT(token_statistics.actual_buy_total + buy_amount <= std::stoll(token.template_parameter.plan_buy_total), 
			"errno=10202011, actual_buy_total + buy_amount > plan_buy_total. fields={\"actual_buy_total\":${a}, \"buy_amount\":${b}, \"plan_buy_total\":${p}}", 
			("a", str_actual_buy_total)("b", str_buy_amount)("p", str_plan_buy_total));

	auto buyer_balance = d.get_balance( op.buyer, asset_id_type() );
	FC_ASSERT( buyer_balance.amount >= pay_amount + op.fee.amount, "errno=10202008, insufficient balance, balance=${balance}, requested pay=${pay}",
              ("balance",buyer_balance)("pay",pay_amount) );

	// check buy times 
	auto& buy_indexes = d.get_index_type<token_buy_index>().indices().get<by_token_id>();
	auto buy_iter 	= buy_indexes.lower_bound( op.token_id ); 
	auto buy_end 	= buy_indexes.upper_bound( op.token_id ); 
	uint buy_times  = 0;

	while(buy_iter != buy_end) {
		const token_buy_object& buy = *buy_iter;
		++buy_iter;

		//同一众筹项目一个用户可认购10次
		if(buy.buyer == op.buyer) 
		{
			++buy_times;
			FC_ASSERT(buy_times < token_profile.buy_max_times, "errno=10202009, everyone can buy ${a} times at most!!!", ("a", token_profile.buy_max_times));
		}
	}

	return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void token_buy_evaluator::pay_fee()
{
	// input to fee pool
	deferred_fee_ = core_fee_paid;
	ilog("buy token ~${fee}", ("fee", deferred_fee_));

}


object_id_type token_buy_evaluator::do_apply( const token_buy_operation& o )
{ try {
	ilog("buy token, id=${id}, buyer=${who}, buy_quantity=${buy_quantity}, phase=${phase}", 
			("id", o.token_id)("who", o.buyer)("buy_quantity", o.template_parameter.buy_quantity)("phase", o.template_parameter.phase));

	return db().apply_token_buy(o, deferred_fee_);

} FC_CAPTURE_AND_RETHROW( (o) ) }


void_result token_event_evaluator::do_evaluate( const token_event_operation& op )
{ try {
	ilog("event token,id=${id}, user=${who}", ("id", op.token_id)("who", op.oper));
	
	// add account lifetime check
	//FC_ASSERT(db().get(op.oper).is_lifetime_member());

	// witness account or committee account
	if(op.oper == GRAPHENE_COMMITTEE_ACCOUNT) { //程序自动产生的event
		return void_result();
	}
	else//手工产生的event
	{
		//auth verify
		FC_ASSERT(trx_state->_is_proposed_trx, "not a proposal transaction, token id=${token_id}, event=${event}", ("token_id", op.token_id)("event", op.event));
	}

/*	auto& witenss_indexes = db().get_index_type<witness_index>().indices().get<by_account>();
	FC_ASSERT(witenss_indexes.find(op.oper) != witenss_indexes.end(), "not witness account");
	
	auto witness = witenss_indexes.find(op.oper);
	const global_property_object& gpo = db().get_global_properties();
	const auto& witnesses = gpo.active_witnesses;
	auto itr_witness = std::find(witnesses.begin(), witnesses.end(), witness->id);
	FC_ASSERT(itr_witness != witnesses.end(), "not active witness account");
*/
	return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }


object_id_type token_event_evaluator::do_apply( const token_event_operation& o )
{ try {
	ilog("event token, id=${id}, user=${who}, event=${event}", ("id", o.token_id)("who", o.oper)("event", o.event));

	const auto& new_token_event_object = db().create<token_event_object>( [&]( token_event_object& obj ){
		
		obj.oper 		= o.oper;
		obj.token_id 	= o.token_id;
		obj.event 		= o.event;
		obj.options 	= o.options;
//		obj.content 	= o.content;
		obj.head_block_number = db().head_block_num();
		obj.time 	= db().head_block_time();
	});

	db().apply_token_event(new_token_event_object);

	return new_token_event_object.id;
} FC_CAPTURE_AND_RETHROW( (o) ) }


/****************************************************************************************
errno
10203001：通证更新操作里没有更新内容
10203002：你指定的更新内容不允许更新
10203003：字段内容长度超出限制
10203004：你没有权限更新通证
10203005：系统内部错误。 当更新字段不是logo_url时，通证对象的exts不能为空
10203006：通证徽章字段超出范围
****************************************************************************************/
void_result token_update_evaluator::do_evaluate( const token_update_operation& op )
{ try {
	ilog("token update do_evaluate, id=${id}, user=${who}", ("id", op.token_id)("who", op.oper));
	
	database& d = db();
	FC_ASSERT(d.find_object(op.token_id));
	auto& index = d.get_index_type<token_index>().indices().get<by_id>();
	auto token_itr = index.find(op.token_id);
	FC_ASSERT( token_itr != index.end() );
	auto& token = *token_itr;

	FC_ASSERT(op.update_key_value.size() > 0, "errno=10203001, there is nothing required to update in your operation!");

	FC_ASSERT( d.get(GRAPHENE_COMMITTEE_ACCOUNT).active.account_auths.count(op.oper),
			"errno=10203004, you has no right to update the token" );

	const module_cfg_object& m = d.get_module_cfg("TOKEN");
	const token_change_profie token_profile = get_token_profile(m.module_cfg);

    for(auto itr = op.update_key_value.begin(); itr != op.update_key_value.end(); ++itr)
    {
    	FC_ASSERT( itr->first == "logo_url" ||
    			itr->first == "brief" ||
    			itr->first == "description" ||
    			itr->first == "official_website" || 
    			itr->first == "poster_url" ||  
    			itr->first == "video_url" || 
    			itr->first == "community" || 
    			itr->first == "risk_prompt" || 
    			itr->first == "faq" ||
    			itr->first == "emblem" ||
    			itr->first == "limit_ratio", 
    			"errno=10203002, the specified update key(${key}) in not allowed!", ("key", itr->first));

    	FC_ASSERT( itr->first == "logo_url" || token.exts, "token.exts can not be null when key is not logo_url");

    	if(itr->first == "brief")
    	{
			//check brief
			string brief = itr->second;
			FC_ASSERT( brief.size() >= token_profile.brief_length.min, "errno=10201059, the length of token brief should contain at least ${min} characters",
					("min", token_profile.brief_length.min));

			FC_ASSERT( brief.size() <= token_profile.brief_length.max, "errno=10201026, the length of token brief can not be longer than ${max}, length=${length}",
					("max", token_profile.brief_length.max)("length", brief.size()));
			
			string lower_case_brief;
			std::transform(brief.begin(), brief.end(), std::back_inserter(lower_case_brief), (int (*)(int))tolower);
			string result = string_contain_sensitive_word(lower_case_brief, d);
			FC_ASSERT( result == "", "errno=10201048, brief contains sensitive_word: ${result}", ("result", result));
    	}
    	else if(itr->first == "description")
    	{
			//check description
			string description = itr->second;
			FC_ASSERT( description.size() <= token_profile.description_length.max, "errno=10201051, the length of token description can not be longer than ${max}",
					("max", token_profile.description_length.max));

			//check if description contains invalid website link
			boost::regex reg(".*<a.*href[ \f\n\r\t\v]*=.*>.*");
			bool match = boost::regex_match(description, reg);
			FC_ASSERT( !match, "errno=10201060, token description contains invalid website link");    		
    	}
		else if (itr->first == "emblem")
		{
		    //check emblem
		    //字符串转成数字进行运算
		    /*
		       通证徽章：
		       0: None
		       1：社区大使通证徽章 
		       2：Dapp通证徽章 
		       4：特邀节点通证徽章
		       7：ALL  (1 + 2 + 4 = 7)
		    */
		    int emblem = atoi((itr->second).c_str());
			FC_ASSERT(0 <= emblem && emblem <= 7, "errno=10203006, the token emblem is out of range. emblem=${emblem}", ("emblem", itr->second));
			
		}
		else if(itr->first == "logo_url")
    	{
			//check logo_url
        	FC_ASSERT( (itr->second).size() <= 1024, "errno=10201008, logo_url is too long");
        	//检查logo_url字段是否为https开头，并符合规范
        	boost::regex https_reg("(https?)://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]");
        	bool if_match = boost::regex_match((itr->second), https_reg);
        	FC_ASSERT( if_match, "errno=10201066, logo_url not https url:${url}", ("url", itr->second));    		
    	}
		else if(itr->first == "poster_url")
    	{
        	//检查poster_url字段是否为https开头，并符合规范
        	boost::regex https_reg("(https?)://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]");
        	bool if_match = boost::regex_match((itr->second), https_reg);
        	FC_ASSERT( if_match, "errno=10201068, poster_url not https url:${url}", ("url", itr->second));    		
    	}

    	FC_ASSERT( itr->first.length() <= 64, "errno=10203003, the length of key field exceeds limit 64. key=${key}", ("key", itr->first));
		FC_ASSERT( itr->second.length() <= 51200, "errno=10203003, the length of value field exceeds limit 51200. key=${key}", ("key", itr->first));
    }

	return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result token_update_evaluator::do_apply( const token_update_operation& op )
{ try {
	ilog("token update, id=${id}, user=${who}", ("id", op.token_id)("who", op.oper));

	database& d = db();
	FC_ASSERT(d.find_object(op.token_id));
	auto& index = d.get_index_type<token_index>().indices().get<by_id>();
	auto token_itr = index.find(op.token_id);
	FC_ASSERT( token_itr != index.end() );
	auto& token = *token_itr;

	d.modify(token, [&](token_object& obj)
	{
		for(auto itr = op.update_key_value.begin(); itr != op.update_key_value.end(); ++itr)
   		{
	    	if( itr->first == "logo_url" )
	    	{
	    		obj.template_parameter.logo_url = itr->second;
	    	}
	    	else if( itr->first == "brief" )
	    	{
	    		obj.template_parameter.brief = itr->second;
	    	}
	    	else if( itr->first == "description" )
	    	{
	    		obj.template_parameter.description = itr->second;
	    	}
	    	else
			{
				(*(obj.exts))[itr->first] = itr->second;
			}
	    }
    });

	return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain


