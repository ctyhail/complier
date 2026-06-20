"""
recursive_descent.py
PL/0 语言递归下降语法分析器
功能：
  1. 实现自顶向下递归下降语法分析
  2. 解析 PL/0 程序（常量、变量、过程、表达式、条件、循环、复合语句）
  3. 输出语法分析结果（正确/错误 + 行号）
  4. FIRST集和FOLLOW集计算
"""

import sys
import os
import importlib
from typing import List, Optional, Dict, Set

# 统一通过 src.lexer.lexical_analyzer 路径加载
_lexer_mod = importlib.import_module('src.lexer.lexical_analyzer')
LexicalAnalyzer = _lexer_mod.LexicalAnalyzer
Token = _lexer_mod.Token
TokenType = _lexer_mod.TokenType
TOKEN_CATEGORY = getattr(_lexer_mod, 'TOKEN_CATEGORY', {})


# ============ 语法树节点 ============

class ASTNode:
    """抽象语法树节点"""
    def __init__(self, name: str, line: int = 0):
        self.name = name
        self.line = line
        self.children: List[ASTNode] = []
        self.token: Optional[Token] = None
    
    def add_child(self, child: 'ASTNode'):
        self.children.append(child)
    
    def print(self, indent: int = 0):
        prefix = '  ' * indent
        if self.token:
            print(f"{prefix}{self.name}: {self.token.value}")
        else:
            print(f"{prefix}{self.name}")
        for child in self.children:
            child.print(indent + 1)


# ============ 语法分析器 ============

class SyntaxError(Exception):
    """语法错误"""
    def __init__(self, message: str, line: int):
        self.line = line
        super().__init__(f"行{line}: {message}")


class RecursiveDescentParser:
    """递归下降语法分析器"""
    
    def __init__(self, lexer: LexicalAnalyzer):
        self.lexer = lexer
        self.tokens: List[Token] = []
        self.current_pos = 0
        self.current_token: Optional[Token] = None
        self.syntax_errors: List[tuple] = []
        self.syntax_tree: Optional[ASTNode] = None
        
        # FIRST 集和 FOLLOW 集
        self.first_sets: Dict[str, Set[TokenType]] = {}
        self.follow_sets: Dict[str, Set[TokenType]] = {}
        self._compute_first_follow()
    
    def _compute_first_follow(self):
        """计算 FIRST 集和 FOLLOW 集"""
        # 非终结符及其产生式（简化版）
        productions = {
            'Program': [['Block', 'DOT']],
            'Block': [['ConstDecl', 'VarDecl', 'ProcDecl', 'Statement']],
            'ConstDecl': [['CONST', 'IdentDefList', 'SEMICOLON'], ['epsilon']],
            'IdentDefList': [['IDENTIFIER', 'EQ', 'NUMBER', 'IdentDefList\'']],
            'IdentDefList\'': [['COMMA', 'IDENTIFIER', 'EQ', 'NUMBER', 'IdentDefList\'']],
            'VarDecl': [['VAR', 'IdentList', 'SEMICOLON'], ['epsilon']],
            'IdentList': [['IDENTIFIER', 'IdentList\'']],
            'IdentList\'': [['COMMA', 'IDENTIFIER', 'IdentList\'']],
            'ProcDecl': [['PROCEDURE', 'IDENTIFIER', 'SEMICOLON', 'Block', 'SEMICOLON', 'ProcDecl'], ['epsilon']],
            'Statement': [['IDENTIFIER', 'ASSIGN', 'Expression'],
                         ['CALL', 'IDENTIFIER'],
                         ['BEGIN', 'StatementList', 'END'],
                         ['IF', 'Condition', 'THEN', 'Statement'],
                         ['WHILE', 'Condition', 'DO', 'Statement'],
                         ['READ', 'LPAREN', 'IDENTIFIER', 'RPAREN'],
                         ['WRITE', 'LPAREN', 'Expression', 'RPAREN'],
                         ['epsilon']],
            'StatementList': [['Statement', 'StatementList\'']],
            'StatementList\'': [['SEMICOLON', 'Statement', 'StatementList\'']],
            'Condition': [['ODD', 'Expression'],
                         ['Expression', 'RelOp', 'Expression']],
            'RelOp': [['EQ'], ['NEQ'], ['LT'], ['LE'], ['GT'], ['GE']],
            'Expression': [['Term', 'Expression\'']],
            'Expression\'': [['PLUS', 'Term', 'Expression\''],
                            ['MINUS', 'Term', 'Expression\'']],
            'Term': [['Factor', 'Term\'']],
            'Term\'': [['TIMES', 'Factor', 'Term\''],
                      ['DIVIDE', 'Factor', 'Term\'']],
            'Factor': [['IDENTIFIER'], ['NUMBER'], ['LPAREN', 'Expression', 'RPAREN']],
        }
        
        # 手动定义的 FIRST 集
        self.first_sets = {
            'Program': {TokenType.CONST, TokenType.VAR, TokenType.PROCEDURE,
                       TokenType.IDENTIFIER, TokenType.CALL, TokenType.BEGIN,
                       TokenType.IF, TokenType.WHILE, TokenType.READ, TokenType.WRITE,
                       TokenType.ODD, TokenType.PLUS, TokenType.MINUS, TokenType.NUMBER,
                       TokenType.LPAREN, TokenType.SEMICOLON, TokenType.END},
            'Block': {TokenType.CONST, TokenType.VAR, TokenType.PROCEDURE,
                     TokenType.IDENTIFIER, TokenType.CALL, TokenType.BEGIN,
                     TokenType.IF, TokenType.WHILE, TokenType.READ, TokenType.WRITE,
                     TokenType.ODD, TokenType.PLUS, TokenType.MINUS, TokenType.NUMBER,
                     TokenType.LPAREN, TokenType.SEMICOLON, TokenType.END},
            'ConstDecl': {TokenType.CONST},
            'VarDecl': {TokenType.VAR},
            'ProcDecl': {TokenType.PROCEDURE},
            'Statement': {TokenType.IDENTIFIER, TokenType.CALL, TokenType.BEGIN,
                         TokenType.IF, TokenType.WHILE, TokenType.READ, TokenType.WRITE,
                         TokenType.ODD, TokenType.PLUS, TokenType.MINUS, TokenType.NUMBER,
                         TokenType.LPAREN, TokenType.SEMICOLON, TokenType.END},
            'Condition': {TokenType.ODD, TokenType.IDENTIFIER, TokenType.NUMBER,
                         TokenType.LPAREN, TokenType.PLUS, TokenType.MINUS},
            'Expression': {TokenType.IDENTIFIER, TokenType.NUMBER, TokenType.LPAREN,
                          TokenType.PLUS, TokenType.MINUS},
            'Term': {TokenType.IDENTIFIER, TokenType.NUMBER, TokenType.LPAREN,
                    TokenType.PLUS, TokenType.MINUS},
            'Factor': {TokenType.IDENTIFIER, TokenType.NUMBER, TokenType.LPAREN},
        }
        
        # FOLLOW 集
        self.follow_sets = {
            'Program': set(),
            'Block': {TokenType.DOT, TokenType.SEMICOLON},
            'ConstDecl': {TokenType.VAR, TokenType.PROCEDURE, TokenType.IDENTIFIER,
                         TokenType.CALL, TokenType.BEGIN, TokenType.IF, TokenType.WHILE,
                         TokenType.READ, TokenType.WRITE, TokenType.DOT},
            'VarDecl': {TokenType.PROCEDURE, TokenType.IDENTIFIER, TokenType.CALL,
                       TokenType.BEGIN, TokenType.IF, TokenType.WHILE, TokenType.READ,
                       TokenType.WRITE, TokenType.DOT},
            'ProcDecl': {TokenType.DOT, TokenType.SEMICOLON},
            'Statement': {TokenType.SEMICOLON, TokenType.END, TokenType.DOT},
            'Condition': {TokenType.THEN, TokenType.DO},
            'Expression': {TokenType.SEMICOLON, TokenType.RPAREN, TokenType.THEN, 
                          TokenType.DO, TokenType.END, TokenType.COMMA, TokenType.EQ,
                          TokenType.NEQ, TokenType.LT, TokenType.LE, TokenType.GT, TokenType.GE},
            'Term': {TokenType.PLUS, TokenType.MINUS, TokenType.SEMICOLON, 
                    TokenType.RPAREN, TokenType.THEN, TokenType.DO, TokenType.END,
                    TokenType.COMMA, TokenType.EQ, TokenType.NEQ, TokenType.LT,
                    TokenType.LE, TokenType.GT, TokenType.GE},
            'Factor': {TokenType.TIMES, TokenType.DIVIDE, TokenType.PLUS, TokenType.MINUS,
                      TokenType.SEMICOLON, TokenType.RPAREN, TokenType.THEN, TokenType.DO,
                      TokenType.END, TokenType.COMMA, TokenType.EQ, TokenType.NEQ,
                      TokenType.LT, TokenType.LE, TokenType.GT, TokenType.GE},
        }
    
    def parse(self, tokens: List[Token]) -> bool:
        """执行语法分析"""
        self.tokens = tokens
        self.current_pos = 0
        self.syntax_errors = []
        self.current_token = tokens[0] if tokens else None
        
        try:
            self.syntax_tree = self._program()
            if self.current_token and self.current_token.type != TokenType.EOF:
                # 检查是否还有未处理的 token（应该是 DOT）
                if self.current_token.type == TokenType.DOT:
                    self.advance()  # 消费 DOT
            return len(self.syntax_errors) == 0
        except SyntaxError as e:
            self.syntax_errors.append((e.line, str(e)))
            return False
    
    def advance(self) -> Token:
        """前进到下一个 Token"""
        self.current_pos += 1
        if self.current_pos < len(self.tokens):
            self.current_token = self.tokens[self.current_pos]
        else:
            self.current_token = Token(TokenType.EOF, '', self.lexer.line, 1)
        return self.current_token
    
    def expect(self, token_type: TokenType, error_msg: str = ""):
        """期望当前 Token 为指定类型"""
        if self.current_token and self.current_token.type == token_type:
            token = self.current_token
            self.advance()
            return token
        else:
            actual = f"'{self.current_token.value}'" if self.current_token else "EOF"
            expected = token_type.name
            line = self.current_token.line if self.current_token else 0
            msg = error_msg or f"期望 {expected}，但得到 {actual}"
            raise SyntaxError(msg, line)
    
    def match(self, token_type: TokenType) -> bool:
        """检查当前 Token 是否匹配"""
        return self.current_token and self.current_token.type == token_type
    
    # ============ 语法规则 ============
    
    def _program(self) -> ASTNode:
        """Program → Block '.'"""
        node = ASTNode('Program')
        node.add_child(self._block())
        self.expect(TokenType.DOT, "程序应以 '.' 结尾")
        return node
    
    def _block(self) -> ASTNode:
        """Block → [ConstDecl] [VarDecl] [ProcDecl] Statement"""
        node = ASTNode('Block')
        
        # 可选常量声明
        if self.match(TokenType.CONST):
            node.add_child(self._const_decl())
        
        # 可选变量声明
        if self.match(TokenType.VAR):
            node.add_child(self._var_decl())
        
        # 可选过程声明
        while self.match(TokenType.PROCEDURE):
            node.add_child(self._proc_decl())
        
        # 语句
        node.add_child(self._statement())
        
        return node
    
    def _const_decl(self) -> ASTNode:
        """ConstDecl → 'const' Ident '=' Number {',' Ident '=' Number} ';'"""
        node = ASTNode('ConstDecl')
        
        self.expect(TokenType.CONST)
        
        # 读取第一个常量定义
        ident = self.expect(TokenType.IDENTIFIER, "常量声明需要标识符")
        ident_node = ASTNode('Ident', ident.line)
        ident_node.token = ident
        node.add_child(ident_node)
        
        self.expect(TokenType.EQ, "常量声明需要 '='")
        
        num = self.expect(TokenType.NUMBER, "常量声明需要数字")
        num_node = ASTNode('Number', num.line)
        num_node.token = num
        node.add_child(num_node)
        
        # 读取更多常量定义
        while self.match(TokenType.COMMA):
            self.advance()
            ident = self.expect(TokenType.IDENTIFIER, "常量声明需要标识符")
            ident_node = ASTNode('Ident', ident.line)
            ident_node.token = ident
            node.add_child(ident_node)
            
            self.expect(TokenType.EQ, "常量声明需要 '='")
            
            num = self.expect(TokenType.NUMBER, "常量声明需要数字")
            num_node = ASTNode('Number', num.line)
            num_node.token = num
            node.add_child(num_node)
        
        self.expect(TokenType.SEMICOLON, "常量声明需要 ';'")
        
        return node
    
    def _var_decl(self) -> ASTNode:
        """VarDecl → 'var' Ident {',' Ident} ';'"""
        node = ASTNode('VarDecl')
        
        self.expect(TokenType.VAR)
        
        ident = self.expect(TokenType.IDENTIFIER, "变量声明需要标识符")
        ident_node = ASTNode('Ident', ident.line)
        ident_node.token = ident
        node.add_child(ident_node)
        
        while self.match(TokenType.COMMA):
            self.advance()
            ident = self.expect(TokenType.IDENTIFIER, "变量声明需要标识符")
            ident_node = ASTNode('Ident', ident.line)
            ident_node.token = ident
            node.add_child(ident_node)
        
        self.expect(TokenType.SEMICOLON, "变量声明需要 ';'")
        
        return node
    
    def _proc_decl(self) -> ASTNode:
        """ProcDecl → 'procedure' Ident ';' Block ';'"""
        node = ASTNode('ProcDecl')
        
        self.expect(TokenType.PROCEDURE)
        ident = self.expect(TokenType.IDENTIFIER, "过程声明需要标识符")
        ident_node = ASTNode('ProcName', ident.line)
        ident_node.token = ident
        node.add_child(ident_node)
        
        self.expect(TokenType.SEMICOLON, "过程声明后需要 ';'")
        node.add_child(self._block())
        self.expect(TokenType.SEMICOLON, "过程体后需要 ';'")
        
        return node
    
    def _statement(self) -> ASTNode:
        """Statement 的语法分析"""
        node = ASTNode('Statement')
        
        if not self.current_token:
            return node
        
        # ε 产生式：空语句
        if self.current_token.type in {TokenType.SEMICOLON, TokenType.END, TokenType.DOT}:
            node.name = 'EmptyStatement'
            return node
        
        line = self.current_token.line
        
        try:
            if self.current_token.type == TokenType.IDENTIFIER:
                # 赋值语句: Ident ':=' Expression
                ident = self.current_token
                self.advance()
                ident_node = ASTNode('Ident', ident.line)
                ident_node.token = ident
                node.add_child(ident_node)
                
                self.expect(TokenType.ASSIGN, f"行{line}: 赋值语句需要 ':='")
                node.add_child(self._expression())
                node.name = 'AssignStmt'
                
            elif self.current_token.type == TokenType.CALL:
                # 调用语句: 'call' Ident
                self.advance()
                ident = self.expect(TokenType.IDENTIFIER, f"行{line}: CALL 后需标识符")
                ident_node = ASTNode('Ident', ident.line)
                ident_node.token = ident
                node.add_child(ident_node)
                node.name = 'CallStmt'
                
            elif self.current_token.type == TokenType.BEGIN:
                # 复合语句: 'begin' Statement {';' Statement} 'end'
                self.advance()
                node.name = 'CompoundStmt'
                
                # 至少要有一个语句，除非立即遇到 'end'
                if not self.match(TokenType.END):
                    node.add_child(self._statement())
                    
                    while self.match(TokenType.SEMICOLON):
                        self.advance()
                        if self.match(TokenType.END):
                            break
                        node.add_child(self._statement())
                
                self.expect(TokenType.END, f"行{line}: BEGIN 需匹配 END")
                
            elif self.current_token.type == TokenType.IF:
                # 条件语句: 'if' Condition 'then' Statement
                self.advance()
                node.add_child(self._condition())
                self.expect(TokenType.THEN, f"行{line}: IF 后需 THEN")
                node.add_child(self._statement())
                node.name = 'IfStmt'
                
            elif self.current_token.type == TokenType.WHILE:
                # 循环语句: 'while' Condition 'do' Statement
                self.advance()
                node.add_child(self._condition())
                self.expect(TokenType.DO, f"行{line}: WHILE 后需 DO")
                node.add_child(self._statement())
                node.name = 'WhileStmt'
                
            elif self.current_token.type == TokenType.READ:
                # 读语句: 'read' '(' Ident ')'
                self.advance()
                self.expect(TokenType.LPAREN, f"行{line}: READ 后需 '('")
                ident = self.expect(TokenType.IDENTIFIER, f"行{line}: READ 中需标识符")
                ident_node = ASTNode('Ident', ident.line)
                ident_node.token = ident
                node.add_child(ident_node)
                self.expect(TokenType.RPAREN, f"行{line}: READ 后需 ')'")
                node.name = 'ReadStmt'
                
            elif self.current_token.type == TokenType.WRITE:
                # 写语句: 'write' '(' Expression ')'
                self.advance()
                self.expect(TokenType.LPAREN, f"行{line}: WRITE 后需 '('")
                node.add_child(self._expression())
                self.expect(TokenType.RPAREN, f"行{line}: WRITE 后需 ')'")
                node.name = 'WriteStmt'
                
            else:
                # 其他情况尝试表达式语句
                node.add_child(self._expression())
                node.name = 'ExprStmt'
        
        except SyntaxError as e:
            self.syntax_errors.append((e.line, str(e)))
            # 尝试恢复：跳过直到遇到语句结束符
            while self.current_token and self.current_token.type not in {
                TokenType.SEMICOLON, TokenType.END, TokenType.DOT,
                TokenType.ELSE, TokenType.THEN, TokenType.DO
            }:
                self.advance()
        
        return node
    
    def _condition(self) -> ASTNode:
        """Condition → 'odd' Expression | Expression RelOp Expression"""
        node = ASTNode('Condition')
        line = self.current_token.line if self.current_token else 0
        
        if self.match(TokenType.ODD):
            self.advance()
            node.add_child(self._expression())
            node.name = 'OddCond'
        else:
            node.add_child(self._expression())
            relop_node = self._rel_op()
            if relop_node:
                node.add_child(relop_node)
                node.add_child(self._expression())
                node.name = 'RelCond'
            else:
                node.name = 'Cond'
        
        return node
    
    def _rel_op(self) -> Optional[ASTNode]:
        """RelOp → '=' | '#' | '<' | '<=' | '>' | '>=' """
        if self.current_token and self.current_token.type in {
            TokenType.EQ, TokenType.NEQ, TokenType.LT, 
            TokenType.LE, TokenType.GT, TokenType.GE
        }:
            node = ASTNode('RelOp', self.current_token.line)
            node.token = self.current_token
            self.advance()
            return node
        return None
    
    def _expression(self) -> ASTNode:
        """Expression → ['+'|'-'] Term {('+'|'-') Term}"""
        node = ASTNode('Expression')
        
        # 可选的前置正负号
        if self.current_token and self.current_token.type in {TokenType.PLUS, TokenType.MINUS}:
            op_node = ASTNode('UnaryOp', self.current_token.line)
            op_node.token = self.current_token
            node.add_child(op_node)
            self.advance()
        
        node.add_child(self._term())
        
        # 后续的加减操作
        while self.current_token and self.current_token.type in {TokenType.PLUS, TokenType.MINUS}:
            op_node = ASTNode('BinOp', self.current_token.line)
            op_node.token = self.current_token
            node.add_child(op_node)
            self.advance()
            node.add_child(self._term())
        
        return node
    
    def _term(self) -> ASTNode:
        """Term → Factor {('*'|'/') Factor}"""
        node = ASTNode('Term')
        node.add_child(self._factor())
        
        while self.current_token and self.current_token.type in {TokenType.TIMES, TokenType.DIVIDE}:
            op_node = ASTNode('BinOp', self.current_token.line)
            op_node.token = self.current_token
            node.add_child(op_node)
            self.advance()
            node.add_child(self._factor())
        
        return node
    
    def _factor(self) -> ASTNode:
        """Factor → Ident | Number | '(' Expression ')' """
        node = ASTNode('Factor')
        line = self.current_token.line if self.current_token else 0
        
        if self.match(TokenType.IDENTIFIER):
            node.token = self.current_token
            self.advance()
        elif self.match(TokenType.NUMBER):
            node.token = self.current_token
            self.advance()
        elif self.match(TokenType.LPAREN):
            self.advance()
            node.add_child(self._expression())
            self.expect(TokenType.RPAREN, f"行{line}: 括号不匹配")
        else:
            if self.current_token:
                raise SyntaxError(f"非法的表达式开始 '{self.current_token.value}'", line)
        
        return node
    
    def print_analysis_result(self):
        """输出语法分析结果"""
        print("\n语法分析结果:")
        if not self.syntax_errors:
            print("语法正确")
        else:
            for line, msg in self.syntax_errors:
                print(f"(语法错误, 行号:{line})")
        
        print("\n语法分析树:")
        if self.syntax_tree:
            self.syntax_tree.print()


# ============ 测试代码 ============

def test_correct_syntax():
    """测试正确的语法"""
    print("=" * 60)
    print("语法分析测试1: 正确的 PL/0 程序")
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
    print("输入:\n" + source)
    
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    
    parser = RecursiveDescentParser(lexer)
    parser.parse(tokens)
    parser.print_analysis_result()


def test_error_syntax():
    """测试错误的语法"""
    print("=" * 60)
    print("语法分析测试2: 含错误 PL/0 程序")
    print("=" * 60)
    
    source = """const a := 10;
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
end
end.
"""
    print("输入:\n" + source)
    
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    
    parser = RecursiveDescentParser(lexer)
    parser.parse(tokens)
    parser.print_analysis_result()


if __name__ == "__main__":
    test_correct_syntax()
    print("\n" + "=" * 60 + "\n")
    test_error_syntax()
