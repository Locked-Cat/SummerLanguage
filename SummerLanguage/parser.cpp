#include "parser.h"
#include <cassert>

namespace summer_lang
{
	parser::parser()
	{
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmParser();
		llvm::InitializeNativeTargetAsmPrinter();
		auto & context = llvm::getGlobalContext();
		JIT_helper = std::make_unique<MCJIT_helper>(context);

		precedence_['<'] = 10;
		precedence_['+'] = 20;
		precedence_['-'] = 20;
		precedence_['*'] = 49;
	}

	void parser::handle_extern()
	{
		if (auto proto_ast = parse_extern_())
		{
			if (auto ir = proto_ast->codegen())
			{
				std::cout << "Parsed an extern";
				ir->dump();
			}
		}
		else
			get_next_token_();
	}

	void parser::handle_function()
	{
		if (auto func_ast = parse_function_())
		{
			if (auto ir = func_ast->codegen())
			{
				std::cout << "Parsed a function";
				ir->dump();
			}
		}
		else
			get_next_token_();
	}

	void parser::handle_top_level_expr()
	{
		if (auto func_ast = parse_top_level_expr_())
		{
			if (auto ir = func_ast->codegen())
			{
				auto p_function = (double(*)())(intptr_t)JIT_helper->get_pointer_to_function(ir);
				std::cout << p_function() << std::endl;
			}
		}
		else
			get_next_token_();
	}

	void parser::parse(const std::string & file_name)
	{
		std::ifstream source_code(file_name.c_str());
		if (!source_code.is_open())
			print_error<>("Can't open file " + file_name);
		else
		{
			p_tokenizer_ = std::make_unique<tokenizer>(source_code);

			get_next_token_();
			while (true)
			{
				switch (current_token_.get_type())
				{
				case token_type::END:
					return;
				case token_type::KEYWORD:
				{
					auto current_keyword = current_token_.get_value<keyword>();
					if (current_keyword == "extern")
						handle_extern();
					else
					{
						if (current_keyword == "function")
							handle_function();
						else
						{
							print_error<>("Unknown keyword");
							get_next_token_();
						}
					}
					break;
				}
				default:
					handle_top_level_expr();
					break;
				}
			}

			p_tokenizer_.reset();
		}
	}

	void parser::get_next_token_()
	{
		current_token_ = p_tokenizer_->get_token();
	}

	int parser::get_op_precedence_(char op)
	{
		if (precedence_.find(op) != precedence_.end())
			return precedence_[op];
		return -1;
	}

	std::unique_ptr<ast> parser::parse_number_()
	{
		auto result = std::make_unique<number_ast>(current_token_.get_value<number>());
		get_next_token_();
		return std::move(result);
	}

	std::unique_ptr<ast> parser::parse_parenthesis_()
	{
		get_next_token_();

		auto result = parse_expression_();
		if (!result)
			return result;

		if (current_token_.get_value<op>() != ')')
			return print_error<std::unique_ptr<ast>>("Expected a ')'");
		get_next_token_();
		return result;
	}

	std::unique_ptr<ast> parser::parse_identifier_()
	{
		auto name = current_token_.get_value<identifier>();

		get_next_token_();
		if (current_token_.get_value<op>() != '(')
			return std::make_unique<variable_ast>(name);
		
		get_next_token_();
		std::vector<std::unique_ptr<ast>> args;
		if (current_token_.get_value<op>() != ')')
		{
			while (true)
			{
				auto arg = parse_expression_();
				if (!arg)
					return nullptr;
				args.push_back(std::move(arg));

				if (current_token_.get_value<op>() == ')')
					break;
				if (current_token_.get_value<op>() != ',')
					return print_error<std::unique_ptr<ast>>("Expect a ')' or ',' in argument list");

				get_next_token_();
			}
		}

		get_next_token_();
		return std::make_unique<call_expression_ast>(name, std::move(args));
	}

	std::unique_ptr<ast> parser::parse_primary_()
	{
		switch (current_token_.get_type())
		{
		case token_type::NUMBER:
			return parse_number_();
		case token_type::IDENTIFIER:
			return parse_identifier_();
		case token_type::KEYWORD:
			if (current_token_.get_value<keyword>() == "if")
				return parse_if_();
			else
				break;
		case token_type::OPERATOR:
			if (current_token_.get_value<op>() == '(')
				return parse_parenthesis_();
			else
				break;
		default:
			break;
		}
		return print_error<std::unique_ptr<ast>>("Unknown token when expection an expression");
	}

	std::unique_ptr<ast> parser::parse_expression_()
	{
		auto left = parse_primary_();
		if (!left)
			return nullptr;

		return parse_bin_op_right_(0, std::move(left));
	}

	std::unique_ptr<ast> parser::parse_bin_op_right_(int expr_precedence, std::unique_ptr<ast> left)
	{
		while (true)
		{
			auto current_op = current_token_.get_value<op>();
			auto current_precedence = get_op_precedence_(current_op);

			if (current_precedence < expr_precedence)
				return left;

			get_next_token_();
			auto right = parse_primary_();
			if (!right)
				return nullptr;

			auto next_op = current_token_.get_value<op>();
			auto next_precedence = get_op_precedence_(next_op);

			if (current_precedence < next_precedence)
			{
				right = parse_bin_op_right_(current_precedence + 1, std::move(right));
				if (!right)
					return nullptr;
			}

			left = std::make_unique<binary_expression_ast>(current_op, std::move(left), std::move(right));
		}
	}

	std::unique_ptr<ast> parser::parse_if_()
	{
		get_next_token_();

		auto cond = parse_expression_();
		if (!cond)
			return nullptr;

		if (current_token_.get_value<keyword>() != "then")
			return print_error<std::unique_ptr<ast>>("Expected then");
		get_next_token_();

		auto then_part = parse_expression_();
		if (!then_part)
			return nullptr;

		if (current_token_.get_value<keyword>() != "else")
			return print_error<std::unique_ptr<ast>>("Expected else");
		get_next_token_();

		auto else_part = parse_expression_();
		if (!else_part)
			return nullptr;

		return std::make_unique<if_expression_ast>(std::move(cond), std::move(then_part), std::move(else_part));

	}

	std::unique_ptr<prototype_ast> parser::parse_prototype_()
	{
		if (current_token_.get_type() != token_type::IDENTIFIER)
			return print_error<std::unique_ptr<prototype_ast>>("Expected function name in prototype");

		auto name = current_token_.get_value<identifier>();
		get_next_token_();

		if (current_token_.get_value<op>() != '(')
			return print_error<std::unique_ptr<prototype_ast>>("Expected '(' in prototype");
		get_next_token_();

		std::vector<std::string> args;
		if (current_token_.get_value<op>() != ')')
		{
			while (true)
			{
				if (current_token_.get_type() != token_type::IDENTIFIER)
					return print_error<std::unique_ptr<prototype_ast>>("Expect a argument name in prototype");

				auto arg = current_token_.get_value<identifier>();
				args.push_back(arg);
				get_next_token_();

				if (current_token_.get_value<op>() == ')')
					break;
				
				if (current_token_.get_value<op>() != ',')
					return print_error<std::unique_ptr<prototype_ast>>("Expect a ')' or ',' in prototype");

				get_next_token_();
			}
		}

		get_next_token_();
		return std::make_unique<prototype_ast>(name, std::move(args));
	}

	std::unique_ptr<function_ast> parser::parse_function_()
	{
		get_next_token_();
		
		auto prototype = parse_prototype_();
		if (!prototype)
			return nullptr;

		auto body = parse_expression_();
		if (!body)
			return nullptr;

		return std::make_unique<function_ast>(std::move(prototype), std::move(body));
	}

	std::unique_ptr<prototype_ast> parser::parse_extern_()
	{
		get_next_token_();

		return parse_prototype_();
	}

	std::unique_ptr<function_ast> parser::parse_top_level_expr_()
	{
		auto prototype = std::make_unique<prototype_ast>("", std::vector<std::string>());
		auto expression = parse_expression_();
		if (!expression)
			return nullptr;

		return std::make_unique<function_ast>(std::move(prototype), std::move(expression));
	}

	llvm::Value * error_v(const std::string & message)
	{
		std::cerr << message << std::endl;
		return nullptr;
	}

	llvm::Value * number_ast::codegen()
	{
		return llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(value_));
	}

	llvm::Value * variable_ast::codegen()
	{
		auto value = global_named_values[name_];
		if (!value)
			return print_error<llvm::Value *>("Unknown variable name");
		return value;
	}

	llvm::Value * binary_expression_ast::codegen()
	{
		auto l_value = left_->codegen();
		auto r_value = right_->codegen();

		if (!l_value || !r_value)
			return nullptr;

		switch (op_)
		{
		case '+':
			return global_builder.CreateFAdd(l_value, r_value, "addtmp");
		case '-':
			return global_builder.CreateFSub(l_value, r_value, "subtmp");
		case '*':
			return global_builder.CreateFMul(l_value, r_value, "multmp");
		case '<':
		{
			auto l = global_builder.CreateFCmpULT(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(l, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		default:
			return print_error<llvm::Value *>("Invalid binary operator");
		}
	}

	llvm::Value * call_expression_ast::codegen()
	{
		auto callee_function = JIT_helper->get_function(callee_);
		if (!callee_function)
			return print_error<llvm::Value *>("Unknown function referenced");

		if (callee_function->arg_size() != args_.size())
			return print_error<llvm::Value *>("Incorrect number of arguments passed");

		std::vector<llvm::Value *> args_value;
		for (auto & arg : args_)
		{
			args_value.push_back(arg->codegen());
			if (!args_value.back())
				return nullptr;
		}

		return global_builder.CreateCall(callee_function, args_value, "calltmp");
	}
	
	llvm::Function * prototype_ast::codegen()
	{
		std::vector<llvm::Type *> doubles(args_.size(), llvm::Type::getDoubleTy(llvm::getGlobalContext()));

		auto function_type = llvm::FunctionType::get(llvm::Type::getDoubleTy(llvm::getGlobalContext()), doubles, false);
		auto module = JIT_helper->get_module_for_new_function();

		auto function_name = MCJIT_helper::generate_function_name(name_);
		auto function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, function_name, module);
		if (function->getName() != function_name)
		{
			function->eraseFromParent();
			function = JIT_helper->get_function(name_);
			if (!function->empty())
				return print_error<llvm::Function *>("Redefinition of function " + name_);
			if (function->arg_size() != args_.size())
				return print_error<llvm::Function *>("Redefinition of function " + name_ + " with different number of args");
		}

		auto id = 0;
		for (auto & arg : function->args())
		{
			arg.setName(args_[id]);
			global_named_values[args_[id++]] = &arg;
		}

		return function;
	}
	
	llvm::Function * function_ast::codegen()
	{
		global_named_values.clear();

		auto function = prototype_->codegen();
		if (!function)
			return nullptr;

		auto basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", function);
		global_builder.SetInsertPoint(basic_block);

		global_named_values.clear();
		for (auto & arg : function->args())
			global_named_values[arg.getName()] = &arg;

		if (auto ret_value = body_->codegen())
		{
			global_builder.CreateRet(ret_value);
			llvm::verifyFunction(*function);
			return function;
		}
		else
		{
			function->eraseFromParent();
			return nullptr;
		}
	}

	llvm::Value * if_expression_ast::codegen()
	{
		auto cond_value = cond_->codegen();

		if (!cond_value)
			return nullptr;

		cond_value = global_builder.CreateFCmpONE(cond_value, llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)), "ifcond");
		auto function = global_builder.GetInsertBlock()->getParent();

		auto then_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "then", function);
		auto else_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "else");
		auto merge_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "merge");

		global_builder.CreateCondBr(cond_value, then_basic_block, else_basic_block);
		global_builder.SetInsertPoint(then_basic_block);
		
		auto then_value = then_part_->codegen();
		if (!then_value)
			return nullptr;
		
		global_builder.CreateBr(merge_basic_block);
		then_basic_block = global_builder.GetInsertBlock();

		function->getBasicBlockList().push_back(else_basic_block);
		global_builder.SetInsertPoint(else_basic_block);

		auto else_value = else_part_->codegen();
		if (!else_value)
			return nullptr;

		global_builder.CreateBr(merge_basic_block);
		else_basic_block = global_builder.GetInsertBlock();

		function->getBasicBlockList().push_back(merge_basic_block);
		global_builder.SetInsertPoint(merge_basic_block);

		auto PHI_node = global_builder.CreatePHI(llvm::Type::getDoubleTy(llvm::getGlobalContext()), 2, "iftmp");
		PHI_node->addIncoming(then_value, then_basic_block);
		PHI_node->addIncoming(else_value, else_basic_block);
		return PHI_node;
	}
}