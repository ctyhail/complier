"""
lr_parser.py
PL/0 语言 LR(1) 自底向上语法分析器
功能：
  1. LR(1) 项目集及识别活前缀的状态转换图自动生成
  2. LR(1) 分析表自动构造
  3. 显示归约/移进操作序列、当前状态和输入符号
  4. 语法分析结果（正确/错误）
"""

import sys
import os
import importlib
from typing import List, Dict, Tuple, Set, Optional
from collections import defaultdict
from dataclasses import dataclass, field

_lexer_mod = importlib.import_module('src.lexer.lexical_analyzer')
LexicalAnalyzer = _lexer_mod.LexicalAnalyzer
Token = _lexer_mod.Token
TokenType = _lexer_mod.TokenType
TOKEN_CATEGORY = getattr(_lexer_mod, 'TOKEN_CATEGORY', {})


# ============ 文法定义 ============

@dataclass(frozen=True)
class Production:
    """产生式"""
    lhs: str       # 左部
    rhs: tuple  # 右部 (用 tuple 保证可哈希)
    index: int = 0  # 产生式编号

    def __repr__(self):
        return f"{self.lhs} → {' '.join(self.rhs) if self.rhs else 'ε'}"


class Grammar:
    """PL/0 文法"""
    
    def __init__(self):
        self.productions: List[Production] = []
        self.nonterminals: Set[str] = set()
        self.terminals: Set[str] = set()
        self.start_symbol: str = "Program'"
        self._build()
    
    def _build(self):
        """构建 PL/0 文法"""
        prods = [
            # 增广文法
            ("Program'", ["Program"]),
            # 程序
            ("Program", ["Block", "."]),
            # 块
            ("Block", ["ConstDecl", "VarDecl", "ProcDecl", "Statement"]),
            # 常量声明
            ("ConstDecl", ["const", "IdList"]),
            ("ConstDecl", []),  # ε
            ("IdList", ["ident", "=", "number"]),
            ("IdList", ["IdList", ",", "ident", "=", "number"]),
            # 变量声明
            ("VarDecl", ["var", "IdentList"]),
            ("VarDecl", []),  # ε
            ("IdentList", ["ident"]),
            ("IdentList", ["IdentList", ",", "ident"]),
            # 过程声明
            ("ProcDecl", ["procedure", "ident", ";", "Block", ";"]),
            ("ProcDecl", []),  # ε
            ("ProcDecl", ["ProcDecl", "procedure", "ident", ";", "Block", ";"]),
            # 语句
            ("Statement", ["ident", ":=", "Expression"]),       # 赋值
            ("Statement", ["call", "ident"]),                    # 调用
            ("Statement", ["begin", "StmtList", "end"]),         # 复合
            ("Statement", ["if", "Condition", "then", "Statement"]),  # 条件
            ("Statement", ["while", "Condition", "do", "Statement"]), # 循环
            ("Statement", ["read", "(", "ident", ")"]),          # 读
            ("Statement", ["write", "(", "Expression", ")"]),    # 写
            ("Statement", []),  # ε
            ("StmtList", ["Statement"]),
            ("StmtList", ["StmtList", ";", "Statement"]),
            # 条件
            ("Condition", ["odd", "Expression"]),
            ("Condition", ["Expression", "=", "Expression"]),
            ("Condition", ["Expression", "#", "Expression"]),
            ("Condition", ["Expression", "<", "Expression"]),
            ("Condition", ["Expression", "<=", "Expression"]),
            ("Condition", ["Expression", ">", "Expression"]),
            ("Condition", ["Expression", ">=", "Expression"]),
            # 表达式
            ("Expression", ["Term"]),
            ("Expression", ["Expression", "addop", "Term"]),
            # 项
            ("Term", ["Factor"]),
            ("Term", ["Term", "mulop", "Factor"]),
            # 因子
            ("Factor", ["ident"]),
            ("Factor", ["number"]),
            ("Factor", ["(", "Expression", ")"]),
        ]
        
        for i, (lhs, rhs) in enumerate(prods):
            self.productions.append(Production(lhs, tuple(rhs), i))
            self.nonterminals.add(lhs)
            for sym in rhs:
                if sym.islower() or sym in ['.', ',', ';', '(', ')', ':=']:
                    self.terminals.add(sym)
                else:
                    # 非终结符首字母大写
                    if sym[0].isupper() or sym == "ident" or sym == "number":
                        self.terminals.add(sym)
                    else:
                        self.nonterminals.add(sym)
        
        # 修正：明确标记终结符
        self.terminals = {
            'const', 'var', 'procedure', 'call', 'begin', 'end',
            'if', 'then', 'while', 'do', 'read', 'write', 'odd',
            'ident', 'number', 'addop', 'mulop',
            ':=', '=', '#', '<', '<=', '>', '>=', ',', ';', '.', '(', ')'
        }
        self.nonterminals = {
            "Program'", "Program", "Block",
            "ConstDecl", "IdList", "VarDecl", "IdentList",
            "ProcDecl", "Statement", "StmtList",
            "Condition", "Expression", "Term", "Factor"
        }


# ============ LR(1) 项目 ============

@dataclass(unsafe_hash=True)
class LR1Item:
    """LR(1) 项目"""
    production: Production
    dot_pos: int          # 点的位置 (0 = 开始, len(rhs) = 规约)
    lookahead: str = '$'  # 向前看符号

    @property
    def is_reduce(self) -> bool:
        return self.dot_pos >= len(self.production.rhs)

    @property
    def current_symbol(self) -> Optional[str]:
        """获取点后的符号"""
        if self.dot_pos < len(self.production.rhs):
            return self.production.rhs[self.dot_pos]
        return None

    def advance(self) -> 'LR1Item':
        """点后移"""
        if not self.is_reduce:
            return LR1Item(self.production, self.dot_pos + 1, self.lookahead)
        return self

    def __repr__(self):
        rhs_str = ' '.join(self.production.rhs) if self.production.rhs else 'ε'
        dot_rhs = rhs_str[:self.dot_pos] + '·' + rhs_str[self.dot_pos:]
        return f"[{self.production.lhs} → {dot_rhs}, {self.lookahead}]"


# ============ LR(1) 分析器 ============

class LR1Parser:
    """LR(1) 语法分析器"""
    
    def __init__(self):
        self.grammar = Grammar()
        self.action: Dict[Tuple[int, str], Tuple[str, int]] = {}
        self.goto: Dict[Tuple[int, str], int] = {}
        self.states: List[Set[LR1Item]] = []
        self.parse_steps: List[Dict] = []
        self.errors: List[Tuple[int, str]] = []
        
        self.first_sets = self._compute_first_sets()
        self._build_automaton()
    
    def _compute_first_sets(self) -> Dict[str, Set[str]]:
        """计算所有非终结符的 FIRST 集（固定点迭代，支持左递归）"""
        first: Dict[str, Set[str]] = {nt: set() for nt in self.grammar.nonterminals}
        
        changed = True
        while changed:
            changed = False
            for prod in self.grammar.productions:
                lhs = prod.lhs
                if not prod.rhs:
                    # ε 产生式
                    if 'ε' not in first[lhs]:
                        first[lhs].add('ε')
                        changed = True
                else:
                    # 处理右部符号序列
                    all_nullable = True
                    for sym in prod.rhs:
                        if sym in self.grammar.terminals:
                            if sym not in first[lhs]:
                                first[lhs].add(sym)
                                changed = True
                            all_nullable = False
                            break
                        else:
                            # 非终结符
                            sym_first = first[sym] - {'ε'}
                            if sym_first - first[lhs]:
                                first[lhs] |= sym_first
                                changed = True
                            if 'ε' not in first[sym]:
                                all_nullable = False
                                break
                    
                    if all_nullable:
                        if 'ε' not in first[lhs]:
                            first[lhs].add('ε')
                            changed = True
        
        return first
    
    def _first_seq(self, symbols: tuple, lookahead_set: Set[str]) -> Set[str]:
        """计算符号序列的 FIRST 集"""
        if not symbols:
            return lookahead_set
        
        result = set()
        all_nullable = True
        
        for sym in symbols:
            if sym in self.grammar.terminals:
                result.add(sym)
                all_nullable = False
                break
            else:
                sym_first = self.first_sets.get(sym, set())
                result |= sym_first - {'ε'}
                if 'ε' not in sym_first:
                    all_nullable = False
                    break
        
        if all_nullable:
            result |= lookahead_set
        
        return result
    
    def _closure(self, items: Set[LR1Item]) -> Set[LR1Item]:
        """计算 LR(1) 项目集闭包"""
        closure = set(items)
        changed = True
        
        while changed:
            changed = False
            new_items = set()
            
            for item in closure:
                if item.is_reduce:
                    continue
                
                B = item.current_symbol
                if B in self.grammar.nonterminals:
                    # β 是点后的剩余符号
                    beta = item.production.rhs[item.dot_pos + 1:]
                    # FIRST(β, lookahead)
                    first_set = self._first_seq(beta, {item.lookahead})
                    
                    for prod in self.grammar.productions:
                        if prod.lhs == B:
                            for la in first_set:
                                new_item = LR1Item(prod, 0, la)
                                if new_item not in closure:
                                    new_items.add(new_item)
            
            if new_items:
                closure |= new_items
                changed = True
        
        return closure
    
    def _goto(self, items: Set[LR1Item], symbol: str) -> Set[LR1Item]:
        """计算 GOTO 函数"""
        goto_items = set()
        for item in items:
            if not item.is_reduce and item.current_symbol == symbol:
                goto_items.add(item.advance())
        return self._closure(goto_items)
    
    def _build_automaton(self):
        """构建 LR(1) 自动机和分析表"""
        # 起始项目
        start_prod = self.grammar.productions[0]
        start_item = LR1Item(start_prod, 0, '$')
        start_set = self._closure({start_item})
        
        self.states = [start_set]
        unmarked = [0]
        
        symbols = self.grammar.terminals | self.grammar.nonterminals
        
        while unmarked:
            state_id = unmarked.pop(0)
            state = self.states[state_id]
            
            # 收集当前状态可识别的符号
            transition_symbols = set()
            for item in state:
                if not item.is_reduce:
                    transition_symbols.add(item.current_symbol)
            
            for symbol in transition_symbols:
                new_set = self._goto(state, symbol)
                if not new_set:
                    continue
                
                # 检查是否已经是已有状态
                found = -1
                for i, existing in enumerate(self.states):
                    if existing == new_set:
                        found = i
                        break
                
                if found == -1:
                    found = len(self.states)
                    self.states.append(new_set)
                    unmarked.append(found)
                
                if symbol in self.grammar.terminals:
                    self.action[(state_id, symbol)] = ('s', found)
                else:
                    self.goto[(state_id, symbol)] = found
        
        # 填充规约动作
        for i, state in enumerate(self.states):
            for item in state:
                if item.is_reduce:
                    if item.production.lhs == "Program'":
                        self.action[(i, '$')] = ('acc', 0)
                    else:
                        key = (i, item.lookahead)
                        if key not in self.action:
                            self.action[key] = ('r', item.production.index)
    
    def parse(self, tokens: List[Token]) -> bool:
        """对词法单元序列进行 LR(1) 语法分析"""
        self.parse_steps = []
        self.errors = []
        
        # 将 Token 转换为内部符号表示
        symbol_stream = []
        for token in tokens:
            s = self._token_to_symbol(token)
            if s:
                symbol_stream.append((s, token))
        
        symbol_stream.append(('$', None))  # 结束符
        
        # 分析栈: (state, symbol)
        stack = [(0, '$')]
        input_pos = 0
        success = True
        
        step_count = 0
        while input_pos < len(symbol_stream):
            state = stack[-1][0]
            lookahead = symbol_stream[input_pos][0]
            
            step_info = {
                'step': step_count + 1,
                'stack': [f"{s}({st})" for st, s in stack],
                'input': ' '.join(s[0] for s in symbol_stream[input_pos:input_pos+5]),
                'action': '',
            }
            
            # 查动作表
            action_key = (state, lookahead)
            if action_key in self.action:
                act, val = self.action[action_key]
                
                if act == 's':
                    # 移进
                    step_info['action'] = f"移进 → 状态{val}"
                    stack.append((val, lookahead))
                    input_pos += 1
                elif act == 'r':
                    # 规约
                    prod = self.grammar.productions[val]
                    rhs_len = len(prod.rhs) if prod.rhs else 0
                    
                    if prod.rhs:
                        for _ in range(rhs_len):
                            stack.pop()
                    
                    step_info['action'] = f"归约: {prod}"
                    
                    # 查 goto
                    goto_state = stack[-1][0]
                    goto_key = (goto_state, prod.lhs)
                    if goto_key in self.goto:
                        stack.append((self.goto[goto_key], prod.lhs))
                    else:
                        self.errors.append((tokens[input_pos].line if input_pos < len(tokens) else 0, 
                                           f"GOTO表查找失败: ({goto_state}, {prod.lhs})"))
                        success = False
                        break
                elif act == 'acc':
                    # 接受
                    step_info['action'] = "接受 (acc)"
                    self.parse_steps.append(step_info)
                    break
            else:
                # 错误
                line = symbol_stream[input_pos][1].line if symbol_stream[input_pos][1] else 0
                self.errors.append((line, f"语法错误: 不期望的符号 '{lookahead}'"))
                step_info['action'] = f"错误: 不期望的符号 '{lookahead}'"
                success = False
                # 错误恢复：跳过当前符号
                input_pos += 1
                continue
            
            self.parse_steps.append(step_info)
            step_count += 1
        
        return success and not self.errors
    
    def _token_to_symbol(self, token: Token) -> Optional[str]:
        """将 Token 转换为语法符号"""
        type_map = {
            TokenType.IDENTIFIER: 'ident',
            TokenType.NUMBER: 'number',
            TokenType.ASSIGN: ':=',
            TokenType.EQ: '=',
            TokenType.NEQ: '#',
            TokenType.LT: '<',
            TokenType.LE: '<=',
            TokenType.GT: '>',
            TokenType.GE: '>=',
            TokenType.PLUS: 'addop',
            TokenType.MINUS: 'addop',
            TokenType.TIMES: 'mulop',
            TokenType.DIVIDE: 'mulop',
            TokenType.LPAREN: '(',
            TokenType.RPAREN: ')',
            TokenType.COMMA: ',',
            TokenType.SEMICOLON: ';',
            TokenType.DOT: '.',
            TokenType.CONST: 'const',
            TokenType.VAR: 'var',
            TokenType.PROCEDURE: 'procedure',
            TokenType.BEGIN: 'begin',
            TokenType.END: 'end',
            TokenType.IF: 'if',
            TokenType.THEN: 'then',
            TokenType.WHILE: 'while',
            TokenType.DO: 'do',
            TokenType.CALL: 'call',
            TokenType.READ: 'read',
            TokenType.WRITE: 'write',
            TokenType.ODD: 'odd',
        }
        return type_map.get(token.type)
    
    def print_parse_table(self):
        """输出 LR 分析表"""
        print("\nLR(1) 分析表:")
        print("-" * 80)
        
        terminals = sorted(t for t in self.grammar.terminals if t != 'ε')
        
        # 表头
        header = "状态\t"
        for t in terminals:
            header += f"{t}\t"
        header += "$\t"
        for nt in sorted(self.grammar.nonterminals):
            if nt != "Program'":
                header += f"{nt}\t"
        print(header)
        print("-" * 80)
        
        for i in range(len(self.states)):
            row = f"{i}\t"
            for t in terminals:
                key = (i, t)
                if key in self.action:
                    a, v = self.action[key]
                    row += f"{a}{v}\t"
                else:
                    row += "\t"
            
            key = (i, '$')
            if key in self.action:
                a, v = self.action[key]
                row += f"{a}\t"
            else:
                row += "\t"
            
            for nt in sorted(self.grammar.nonterminals):
                if nt == "Program'":
                    continue
                key = (i, nt)
                if key in self.goto:
                    row += f"{self.goto[key]}\t"
                else:
                    row += "\t"
            
            print(row)
    
    def print_parse_steps(self):
        """输出分析步骤"""
        print("\n语法分析步骤:")
        print("-" * 100)
        print(f"{'步骤':<6} {'状态栈':<30} {'输入':<20} {'动作':<40}")
        print("-" * 100)
        for step in self.parse_steps:
            stack_str = ' '.join(step['stack'][-8:])
            print(f"{step['step']:<6} {stack_str:<30} {step['input']:<20} {step['action']:<40}")
    
    def print_result(self):
        """输出分析结果"""
        if not self.errors:
            print("\n语法正确")
        else:
            for line, msg in self.errors:
                print(f"(语法错误, 行号:{line})")


# ============ 测试 ============

def test_lr_parser():
    """测试 LR(1) 语法分析器"""
    print("=" * 60)
    print("LR(1) 语法分析测试")
    print("=" * 60)
    
    source = """const a = 10;
var b, c;
procedure p;
if a <= 10 then
begin
    c := b + a;
end;
begin
    read(b);
    while b # 0 do
    begin
        call p;
        write(2 * c);
        read(b);
    end
end.
"""
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    
    print(f"\n词法分析结果: {len(tokens)} token")
    lexer.print_tokens()
    
    parser = LR1Parser()
    print(f"\nLR(1) 状态数: {len(parser.states)}")
    
    success = parser.parse(tokens)
    parser.print_parse_steps()
    parser.print_result()


if __name__ == "__main__":
    test_lr_parser()
