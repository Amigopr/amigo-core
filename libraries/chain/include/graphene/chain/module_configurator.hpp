/*
 * Copyright (c) 2015 Amigo, Inc., and contributors.
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
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

   class database;
   class configurator
   {
   public:
	   virtual ~configurator(){}

	   virtual void_result evaluate(const module_cfg_operation& o) = 0;

	   virtual void_result apply(const module_cfg_operation& o) = 0;

	   virtual void set_database(database* db){_db = db;}
	protected:
		database* _db;
   };

   template<typename ConfiguratorType>
   class configurator_impl : public configurator
   {
   public:
	   void_result evaluate(const module_cfg_operation& o) override
	   {
			//module specific evaluation
		    ConfiguratorType configurator(*_db);
		    return configurator.evaluate(o);
	   }

	   void_result apply(const module_cfg_operation& o) override
	   {
			//module specific apply
		    ConfiguratorType configurator(*_db);
		    return configurator.apply(o);
	   }
   };

   template<typename DerivedConfiguratorType>
   class module_configurator
   {
   public:
	   module_configurator(database& db): _db(db) {};
	   virtual ~module_configurator(){};

	   //module specific evaluation
	   virtual void_result evaluate(const module_cfg_operation& o) = 0;

	   //module specific apply
	   virtual void_result apply(const module_cfg_operation& o) = 0;

	protected:
		database& _db;
   };

   // TOKEN参数配置模块
   class token_configurator : public module_configurator<token_configurator>
   {
   public:
        static const string name;

        token_configurator(database& db): module_configurator(db){};
        virtual ~token_configurator(){};

        void_result evaluate(const module_cfg_operation& o) override;

        void_result apply(const module_cfg_operation& o) override;
   };

   // 敏感词参数配置模块
   class word_configurator : public module_configurator<word_configurator>
   {
   public:
        static const string name;

        word_configurator(database& db): module_configurator(db){};
        virtual ~word_configurator(){};

        void_result evaluate(const module_cfg_operation& o) override;

        void_result apply(const module_cfg_operation& o) override;
   };

   // 操作黑名单参数配置模块
   class operation_blacklist_configurator : public module_configurator<operation_blacklist_configurator>
   {
   public:
        static const string name;

        operation_blacklist_configurator(database& db): module_configurator(db){};
        virtual ~operation_blacklist_configurator(){};

        void_result evaluate(const module_cfg_operation& o) override;

        void_result apply(const module_cfg_operation& o) override;
   };
} } // graphene::chain
