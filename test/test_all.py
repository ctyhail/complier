"""
test_all.py
编译原理课程设计综合测试
验证：词法分析、语法分析、语义分析所有模块
"""

import sys
import os

# 添加项目根目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from src.lexer.lexical_analyzer import LexicalAnalyzer, test_correct_program, test_error_program
from src.lexer.nfa_dfa import demo_automaton
from src.parser.recursive_descent import RecursiveDescentParser, test_correct_syntax, test_error_syntax
from src.semantic.semantic_analyzer import SemanticAnalyzer, test_semantic_correct, test_semantic_error


def test_lexical_analysis():
    """词法分析测试"""
    print("\n" + "=" * 70)
    print("第4章：词法分析程序测试")
    print("=" * 70)
    
    # 测试1: 正确程序
    print("\n>>> 测试1: 正确的 PL/0 程序 <<<")
    test_correct_program()
    
    # 测试2: 含错误程序
    print("\n>>> 测试2: 含错误的 PL/0 程序 <<<")
    test_error_program()
    
    # 测试3: NFA → DFA → 最小化
    print("\n>>> 测试3: 正规式→NFA→DFA→最小化 <<<")
    demo_automaton()


def test_syntax_analysis():
    """语法分析测试"""
    print("\n" + "=" * 70)
    print("第5章：语法分析程序测试")
    print("=" * 70)
    
    # 测试1: 正确语法
    print("\n>>> 测试1: 正确的 PL/0 语法 <<<")
    test_correct_syntax()
    
    # 测试2: 错误语法
    print("\n>>> 测试2: 含错误的 PL/0 语法 <<<")
    test_error_syntax()


def test_semantic_analysis():
    """语义分析测试"""
    print("\n" + "=" * 70)
    print("第6章：语义分析程序测试")
    print("=" * 70)
    
    # 测试1: 正确语义
    print("\n>>> 测试1: 正确的 PL/0 语义 <<<")
    test_semantic_correct()
    
    # 测试2: 语义错误
    print("\n>>> 测试2: 含语义错误的 PL/0 程序 <<<")
    test_semantic_error()


def run_all_tests():
    """运行所有测试"""
    print("=" * 70)
    print("编译原理课程设计 - 完整测试套件")
    print(("Python " + sys.version).center(70))
    print("=" * 70)
    
    test_lexical_analysis()
    test_syntax_analysis()
    test_semantic_analysis()
    
    print("\n" + "=" * 70)
    print("所有测试完成！")
    print("=" * 70)


if __name__ == "__main__":
    run_all_tests()
