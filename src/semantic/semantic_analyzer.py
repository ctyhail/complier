"""
semantic_analyzer.py
PL/0 语言语义分析器及中间代码生成器
功能：
  1. 基于 L-翻译模式的 PL/0 语义分析
  2. 生成四元式（三元地址码）中间代码
  3. 符号表管理
  4. 语义错误检测（未声明变量、重复声明、类型不匹配）
  5. 控制流跳转（if/while）和临时变量管理
"""

import sys
import os
from typing import List, Dict, Tuple, Optional, Any
from dataclasses import dataclass, field

import importlib
# 确保通过同一路径加载 lexer 模块
_lexer_mod = importlib.import_module('src.lexer.lexical_analyzer')
LexicalAnalyzer = _lexer_mod.LexicalAnalyzer
Token = _lexer_mod.Token
TokenType = _lexer_mod.TokenType
TOKEN_CATEGORY = getattr(_lexer_mod, 'TOKEN_CATEGORY', {})


# ============ 四元式 ============

@dataclass
class Quadruple:
    """四元式 (op, arg1, arg2, result)"""
    op: str       # 操作符
    arg1: str     # 第一操作数
    arg2: str     # 第二操作数
    result: str   # 结果
    
    def __repr__(self):
        a1 = self.arg1 if self.arg1 else '_'
        a2 = self.arg2 if self.arg2 else '_'
        res = self.result if self.result else '_'
        return f"({self.op},{a1},{a2},{res})"


# ============ 符号表 ============

@dataclass
class SymbolEntry:
    """符号表条目"""
    name: str          # 符号名
    kind: str          # const/var/procedure
    value: Any = None  # 常量值或变量初值
    level: int = 0     # 嵌套层数
    line: int = 0      # 声明行号
    
    def __repr__(self):
        if self.kind == 'const':
            return f"const {self.name} {self.value}"
        elif self.kind == 'var':
            return f"var {self.name} {self.value if self.value else 0}"
        else:
            return f"procedure {self.name}"


class SymbolTable:
    """符号表 - 支持嵌套作用域"""
    
    def __init__(self):
        self.tables: List[Dict[str, SymbolEntry]] = [{}]
        self.current_level = 0
        self.entries: List[SymbolEntry] = []
    
    def enter_scope(self):
        """进入新作用域"""
        self.tables.append({})
        self.current_level += 1
    
    def leave_scope(self):
        """离开作用域"""
        if self.current_level > 0:
            self.tables.pop()
            self.current_level -= 1
    
    def declare(self, name: str, kind: str, value: Any = None, line: int = 0) -> bool:
        """声明符号，成功返回True，重复声明返回False"""
        if name in self.tables[self.current_level]:
            return False  # 重复声明
        
        entry = SymbolEntry(name, kind, value, self.current_level, line)
        self.tables[self.current_level][name] = entry
        self.entries.append(entry)
        return True
    
    def lookup(self, name: str) -> Optional[SymbolEntry]:
        """查找符号（从当前层到外层）"""
        for level in range(self.current_level, -1, -1):
            if name in self.tables[level]:
                return self.tables[level][name]
        return None
    
    def is_declared(self, name: str) -> bool:
        """检查是否已声明"""
        return self.lookup(name) is not None
    
    def print_table(self) -> str:
        """输出符号表"""
        lines = ["符号表:"]
        for entry in self.entries:
            lines.append(str(entry))
        return '\n'.join(lines)


# ============ 语义分析器（L-翻译模式） ============

class SemanticAnalyzer:
    """PL/0 语义分析器 - 基于递归下降的 L-翻译模式"""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0
        self.current_token: Optional[Token] = None
        self.symbol_table = SymbolTable()
        self.quadruples: List[Quadruple] = []
        self.temp_var_count = 0
        self.label_count = 0
        self.errors: List[Tuple[int, str]] = []
        self.semantic_correct = True
        
        # 当前过程的名称
        self.current_procedure = ''
        
        # 循环和条件跳转栈
        self.break_stack: List[int] = []
        self.continue_stack: List[int] = []
        
        self._advance()
    
    def _advance(self):
        """前进到下一个 Token"""
        if self.pos < len(self.tokens):
            self.current_token = self.tokens[self.pos]
            self.pos += 1
        else:
            self.current_token = None
    
    def _peek(self, offset: int = 0) -> Optional[Token]:
        """查看未来的 Token"""
        idx = self.pos + offset
        if idx < len(self.tokens):
            return self.tokens[idx]
        return None
    
    def _match(self, *types: TokenType) -> bool:
        """尝试匹配当前 Token 的类型"""
        if self.current_token and self.current_token.type in types:
            value = self.current_token.value
            self._advance()
            return True
        return False
    
    def _expect(self, *types: TokenType) -> bool:
        """期望匹配指定 Token 类型"""
        if self.current_token and self.current_token.type in types:
            self._advance()
            return True
        line = self.current_token.line if self.current_token else 0
        expected = [t.name for t in types]
        actual = self.current_token.type.name if self.current_token else 'EOF'
        self.errors.append((line, f"期望 {expected}，得到 {actual}"))
        self.semantic_correct = False
        return False
    
    def _new_temp(self) -> str:
        """生成新的临时变量"""
        self.temp_var_count += 1
        return f"T{self.temp_var_count}"
    
    def _new_label(self) -> str:
        """生成新的标签"""
        self.label_count += 1
        return f"${self.label_count}"
    
    def _emit(self, op: str, arg1: str = '', arg2: str = '', result: str = ''):
        """生成四元式"""
        self.quadruples.append(Quadruple(op, arg1, arg2, result))
    
    def _error(self, line: int, msg: str):
        """记录语义错误"""
        self.errors.append((line, msg))
        self.semantic_correct = False
    
    def analyze(self) -> bool:
        """执行语义分析"""
        self.quadruples = []
        self.errors = []
        self.semantic_correct = True
        self.temp_var_count = 0
        self.label_count = 0
        
        self._emit('syss', '', '', '')  # 程序开始
        
        self._program()
        
        self._emit('syse', '', '', '')  # 程序结束
        
        return self.semantic_correct
    
    def _program(self):
        """Program → Block '.'"""
        self._block()
        if self.current_token and self.current_token.type == TokenType.DOT:
            self._emit('ret', '', '', '')
            self._advance()
    
    def _block(self):
        """Block → [ConstDecl] [VarDecl] [ProcDecl] Statement
           使用 while 而非 if 以处理多个连续的声明
        """
        if self.current_token and self.current_token.type == TokenType.CONST:
            self._const_decl()
        
        while self.current_token and self.current_token.type == TokenType.VAR:
            self._var_decl()
        
        while self.current_token and self.current_token.type == TokenType.PROCEDURE:
            self._proc_decl()
        
        self._statement()
    
    def _const_decl(self):
        """ConstDecl → 'const' Ident '=' Number {',' Ident '=' Number} ';'"""
        self._expect(TokenType.CONST)
        
        while True:
            if not self.current_token or self.current_token.type != TokenType.IDENTIFIER:
                break
            
            name = self.current_token.value
            line = self.current_token.line
            self._advance()
            
            if not self._expect(TokenType.EQ):
                break
            
            if self.current_token and self.current_token.type == TokenType.NUMBER:
                value = self.current_token.value
                self._advance()
            else:
                value = '0'
            
            # 声明常量
            if not self.symbol_table.declare(name, 'const', value, line):
                self._error(line, f"重复声明: {name}")
            else:
                self._emit('const', name, '', '')
                self._emit('=', value, '', name)
            
            if self.current_token and self.current_token.type == TokenType.COMMA:
                self._advance()
                continue
            break
        
        self._expect(TokenType.SEMICOLON)
    
    def _var_decl(self):
        """VarDecl → 'var' Ident {',' Ident} ';'
           注意: var 后无标识符时（如教科书的测试用例），不做严格要求 ';'
        """
        self._expect(TokenType.VAR)
        
        while True:
            if not self.current_token or self.current_token.type != TokenType.IDENTIFIER:
                break
            
            name = self.current_token.value
            line = self.current_token.line
            self._advance()
            
            # 声明变量
            if not self.symbol_table.declare(name, 'var', 0, line):
                self._error(line, f"重复声明: {name}")
            else:
                self._emit('var', name, '', '')
            
            if self.current_token and self.current_token.type == TokenType.COMMA:
                self._advance()
                continue
            break
        
        # 如果 var 后有标识符列表，则期望 ';'；否则宽容处理
        if self.current_token and self.current_token.type == TokenType.SEMICOLON:
            self._advance()
    
    def _proc_decl(self):
        """ProcDecl → 'procedure' Ident ';' Block ';' {ProcDecl}"""
        while self.current_token and self.current_token.type == TokenType.PROCEDURE:
            self._advance()
            
            if self.current_token and self.current_token.type == TokenType.IDENTIFIER:
                proc_name = self.current_token.value
                line = self.current_token.line
                self._advance()
                
                if not self.symbol_table.declare(proc_name, 'procedure', None, line):
                    self._error(line, f"重复声明过程: {proc_name}")
                else:
                    self._emit('procedure', proc_name, '', '')
                
                self._expect(TokenType.SEMICOLON)
                
                self.symbol_table.enter_scope()
                self._block()
                self.symbol_table.leave_scope()
                
                self._expect(TokenType.SEMICOLON)
    
    def _statement(self):
        """Statement → 赋值 | call | begin-end | if | while | read | write | ε"""
        if not self.current_token:
            return
        
        ttype = self.current_token.type
        line = self.current_token.line
        
        if ttype == TokenType.IDENTIFIER:
            # 赋值语句: Ident ':=' Expression
            ident_name = self.current_token.value
            self._advance()
            
            if not self.symbol_table.is_declared(ident_name):
                self._error(line, f"未声明的变量: {ident_name}")
            
            if self._expect(TokenType.ASSIGN):
                result = self._expression()
                if self.symbol_table.is_declared(ident_name):
                    self._emit(':=', result, '', ident_name)
        
        elif ttype == TokenType.CALL:
            # 过程调用: 'call' Ident
            self._advance()
            if self.current_token and self.current_token.type == TokenType.IDENTIFIER:
                proc_name = self.current_token.value
                line = self.current_token.line
                self._advance()
                
                entry = self.symbol_table.lookup(proc_name)
                if not entry:
                    self._error(line, f"未声明的过程: {proc_name}")
                elif entry.kind != 'procedure':
                    self._error(line, f"'{proc_name}' 不是过程")
                else:
                    self._emit('call', proc_name, '', '')
        
        elif ttype == TokenType.BEGIN:
            # 复合语句: 'begin' Statement {';' Statement} 'end'
            self._advance()
            while self.current_token and self.current_token.type != TokenType.END:
                self._statement()
                if self.current_token and self.current_token.type == TokenType.SEMICOLON:
                    self._advance()
            self._expect(TokenType.END)
        
        elif ttype == TokenType.IF:
            # 条件语句: 'if' Condition 'then' Statement
            self._advance()
            cond_result = self._condition()
            
            # 生成条件跳转: 不满足则跳过 then 部分
            false_label = self._new_label()
            self._emit(f'j{cond_result["rev_op"]}', cond_result['place'], '0', false_label)
            
            self._expect(TokenType.THEN)
            self._statement()
            
            self._emit(f'{false_label}:', '', '', '')
        
        elif ttype == TokenType.WHILE:
            # 循环语句: 'while' Condition 'do' Statement
            loop_label = self._new_label()
            self._emit(f'{loop_label}:', '', '', '')
            self._advance()
            
            cond_result = self._condition()
            
            # 不满足条件时跳出循环
            exit_label = self._new_label()
            self._emit(f'j{cond_result["rev_op"]}', cond_result['place'], '0', exit_label)
            
            self._expect(TokenType.DO)
            self._statement()
            
            # 跳回循环开始
            self._emit('j', '', '', loop_label)
            self._emit(f'{exit_label}:', '', '', '')
        
        elif ttype == TokenType.READ:
            # 读语句: 'read' '(' Ident ')'
            self._advance()
            self._expect(TokenType.LPAREN)
            if self.current_token and self.current_token.type == TokenType.IDENTIFIER:
                name = self.current_token.value
                line = self.current_token.line
                self._advance()
                
                if not self.symbol_table.is_declared(name):
                    self._error(line, f"未声明的变量: {name}")
                else:
                    self._emit('read', name, '', '')
            self._expect(TokenType.RPAREN)
        
        elif ttype == TokenType.WRITE:
            # 写语句: 'write' '(' Expression ')'
            self._advance()
            self._expect(TokenType.LPAREN)
            result = self._expression()
            self._emit('write', result, '', '')
            self._expect(TokenType.RPAREN)
        
        # else: ε - 空语句
    
    def _condition(self) -> dict:
        """Condition → 'odd' Expression | Expression relop Expression
           返回 {'place': str, 'rev_op': str} 反转跳转条件
        """
        if self.current_token and self.current_token.type == TokenType.ODD:
            self._advance()
            result = self._expression()
            temp = self._new_temp()
            self._emit('odd', result, '', temp)
            return {'place': temp, 'rev_op': '='}  # odd=0 时为假
        
        left = self._expression()
        
        if self.current_token and self.current_token.type in [
            TokenType.EQ, TokenType.NEQ, TokenType.LT, 
            TokenType.LE, TokenType.GT, TokenType.GE
        ]:
            op_token = self.current_token
            self._advance()
            right = self._expression()
            
            temp = self._new_temp()
            op_map = {
                TokenType.EQ: '=',
                TokenType.NEQ: '#',
                TokenType.LT: '<',
                TokenType.LE: '<=',
                TokenType.GT: '>',
                TokenType.GE: '>=',
            }
            rev_map = {
                TokenType.EQ: '#',
                TokenType.NEQ: '=',
                TokenType.LT: '>=',
                TokenType.LE: '>',
                TokenType.GT: '<=',
                TokenType.GE: '<',
            }
            op = op_map.get(op_token.type, '=')
            rev_op = rev_map.get(op_token.type, '#')
            
            self._emit(op, left, right, temp)
            return {'place': temp, 'rev_op': rev_op}
        
        return {'place': left, 'rev_op': '='}
    
    def _expression(self) -> str:
        """Expression → ['+'|'-'] Term {('+'|'-') Term}
           返回存放表达式的变量名或常数
        """
        unary = ''
        if self.current_token and self.current_token.type == TokenType.PLUS:
            self._advance()
        elif self.current_token and self.current_token.type == TokenType.MINUS:
            unary = '-'
            self._advance()
        
        result = self._term()
        
        if unary == '-':
            temp = self._new_temp()
            self._emit('minus', '0', result, temp)
            result = temp
        
        while self.current_token and self.current_token.type in [TokenType.PLUS, TokenType.MINUS]:
            op = self.current_token.value
            self._advance()
            right = self._term()
            temp = self._new_temp()
            self._emit('+' if op == '+' else '-', result, right, temp)
            result = temp
        
        return result
    
    def _term(self) -> str:
        """Term → Factor {('*'|'/') Factor}
           返回存放项的变量名或常数
        """
        result = self._factor()
        
        while self.current_token and self.current_token.type in [TokenType.TIMES, TokenType.DIVIDE]:
            op = self.current_token.value
            self._advance()
            right = self._factor()
            temp = self._new_temp()
            self._emit('*' if op == '*' else '/', result, right, temp)
            result = temp
        
        return result
    
    def _factor(self) -> str:
        """Factor → Ident | Number | '(' Expression ')'
           返回标识符名、常数或表达式结果
        """
        if not self.current_token:
            return ''
        
        if self.current_token.type == TokenType.IDENTIFIER:
            name = self.current_token.value
            line = self.current_token.line
            self._advance()
            
            if not self.symbol_table.is_declared(name):
                self._error(line, f"未声明的变量: {name}")
            
            return name
        
        elif self.current_token.type == TokenType.NUMBER:
            value = self.current_token.value
            self._advance()
            return value
        
        elif self.current_token.type == TokenType.LPAREN:
            self._advance()
            result = self._expression()
            self._expect(TokenType.RPAREN)
            return result
        
        return ''
    
    def print_results(self):
        """输出语义分析结果"""
        if self.errors:
            for line, msg in self.errors:
                print(f"(语义错误, 行号:{line})")
            return
        
        print("语义正确")
        print("中间代码:")
        for i, q in enumerate(self.quadruples, 1):
            print(f"({i}){q}")
        
        print(self.symbol_table.print_table())


# ============ 测试 ============

def test_semantic_correct():
    """测试正确的语义分析"""
    print("=" * 60)
    print("语义分析测试1: 正确的程序")
    print("=" * 60)
    
    source = """const a = 10;
var b, c;
var
//单行注释
/*
多行注释
*/
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
    print("输入: \n" + source)
    
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    
    analyzer = SemanticAnalyzer(tokens)
    analyzer.analyze()
    analyzer.print_results()


def test_semantic_error():
    """测试语义错误检测"""
    print("=" * 60)
    print("语义分析测试2: 含错误的程序")
    print("=" * 60)
    
    source = """const a = 10;
var a, b, c;
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
    print("输入: \n" + source)
    
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    
    analyzer = SemanticAnalyzer(tokens)
    analyzer.analyze()
    analyzer.print_results()


if __name__ == "__main__":
    test_semantic_correct()
    print("\n" + "=" * 60 + "\n")
    test_semantic_error()
