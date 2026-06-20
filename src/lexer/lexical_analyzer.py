"""
lexical_analyzer.py
PL/0 语言词法分析器
功能：
  1. 识别关键字、标识符、数字、运算符、界符
  2. 忽略 /* */ 和 // 注释
  3. 错误处理：非法字符、非法单词、长度超限
  4. 输出词法单元及类别
  5. 输出到文件供语法分析使用
"""

import sys
import os
from typing import List, Tuple, Optional
from enum import Enum, auto


# ============ 词法单元类型定义 ============

class TokenType(Enum):
    """PL/0 词法单元类型"""
    # 关键字
    CONST = auto()
    VAR = auto()
    PROCEDURE = auto()
    BEGIN = auto()
    END = auto()
    IF = auto()
    THEN = auto()
    WHILE = auto()
    DO = auto()
    CALL = auto()
    READ = auto()
    WRITE = auto()
    ODD = auto()
    
    # 标识符
    IDENTIFIER = auto()
    
    # 常数
    NUMBER = auto()
    
    # 运算符
    ASSIGN = auto()       # :=
    EQ = auto()           # =
    NEQ = auto()          # #
    LT = auto()           # <
    LE = auto()           # <=
    GT = auto()           # >
    GE = auto()           # >=
    PLUS = auto()         # +
    MINUS = auto()        # -
    TIMES = auto()        # *
    DIVIDE = auto()       # /
    
    # 界符
    LPAREN = auto()       # (
    RPAREN = auto()       # )
    COMMA = auto()        # ,
    SEMICOLON = auto()    # ;
    DOT = auto()          # .
    
    # 特殊
    EOF = auto()
    ERROR = auto()


# 关键字集合
KEYWORDS = {
    'const': TokenType.CONST,
    'var': TokenType.VAR,
    'procedure': TokenType.PROCEDURE,
    'begin': TokenType.BEGIN,
    'end': TokenType.END,
    'if': TokenType.IF,
    'then': TokenType.THEN,
    'while': TokenType.WHILE,
    'do': TokenType.DO,
    'call': TokenType.CALL,
    'read': TokenType.READ,
    'write': TokenType.WRITE,
    'odd': TokenType.ODD,
}

# Token 类别名（用于输出）
TOKEN_CATEGORY = {
    TokenType.CONST: '保留字',
    TokenType.VAR: '保留字',
    TokenType.PROCEDURE: '保留字',
    TokenType.BEGIN: '保留字',
    TokenType.END: '保留字',
    TokenType.IF: '保留字',
    TokenType.THEN: '保留字',
    TokenType.WHILE: '保留字',
    TokenType.DO: '保留字',
    TokenType.CALL: '保留字',
    TokenType.READ: '保留字',
    TokenType.WRITE: '保留字',
    TokenType.ODD: '保留字',
    TokenType.IDENTIFIER: '标识符',
    TokenType.NUMBER: '无符号整数',
    TokenType.ASSIGN: '运算符',
    TokenType.EQ: '运算符',
    TokenType.NEQ: '运算符',
    TokenType.LT: '运算符',
    TokenType.LE: '运算符',
    TokenType.GT: '运算符',
    TokenType.GE: '运算符',
    TokenType.PLUS: '运算符',
    TokenType.MINUS: '运算符',
    TokenType.TIMES: '运算符',
    TokenType.DIVIDE: '运算符',
    TokenType.LPAREN: '界符',
    TokenType.RPAREN: '界符',
    TokenType.COMMA: '界符',
    TokenType.SEMICOLON: '界符',
    TokenType.DOT: '界符',
    TokenType.EOF: '文件结束',
    TokenType.ERROR: '错误',
}


class Token:
    """词法单元"""
    def __init__(self, token_type: TokenType, value: str, line: int, column: int, 
                 error_type: str = ''):
        self.type = token_type
        self.value = value
        self.line = line
        self.column = column
        self.error_type = error_type  # 用于错误：'非法字符', '标识符长度超长', '无符号整数越界' 等
    
    def __repr__(self):
        if self.type == TokenType.ERROR:
            if self.error_type:
                return f'({self.error_type},{self.value},行号:{self.line})'
            return f'(非法字符(串),{self.value},行号:{self.line})'
        return f'({TOKEN_CATEGORY[self.type]},{self.value})'
    
    def to_file_format(self) -> str:
        """文件输出格式"""
        if self.type == TokenType.ERROR:
            if self.error_type:
                return f'({self.error_type},{self.value},行号:{self.line})'
            return f'(非法字符(串),{self.value},行号:{self.line})'
        return f'({TOKEN_CATEGORY[self.type]},{self.value})'


# ============ 词法分析器 ============

class LexicalError(Exception):
    """词法错误"""
    def __init__(self, message: str, line: int, column: int):
        self.message = message
        self.line = line
        self.column = column
        super().__init__(f"行{line}:{column} - {message}")


class LexicalAnalyzer:
    """PL/0 词法分析器"""
    
    def __init__(self, source: str = "", source_file: str = ""):
        """
        初始化词法分析器
        :param source: 直接传入源代码字符串
        :param source_file: 或从文件读取源代码
        """
        if source_file:
            with open(source_file, 'r', encoding='utf-8') as f:
                self.source = f.read()
        else:
            self.source = source
        
        self.source += '\n'  # 添加末尾换行
        self.pos = 0
        self.line = 1
        self.column = 1
        self.tokens: List[Token] = []
        self.errors: List[Token] = []
        
        # 单词分类表
        self.word_class_table = self._build_word_class_table()
        # 状态转换图数据
        self.state_transition = self._build_state_transition()
    
    def _build_word_class_table(self) -> List[dict]:
        """构建单词分类表"""
        table = []
        for token_type in TokenType:
            if token_type == TokenType.EOF or token_type == TokenType.ERROR:
                continue
            category = TOKEN_CATEGORY[token_type]
            name = token_type.name
            example = self._get_example(token_type)
            table.append({
                '类别': category,
                '名称': name,
                '例子': example
            })
        return table
    
    def _get_example(self, token_type: TokenType) -> str:
        """获取每种 token 的例子"""
        examples = {
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
            TokenType.IDENTIFIER: 'x, count',
            TokenType.NUMBER: '123, 0',
            TokenType.ASSIGN: ':=',
            TokenType.EQ: '=',
            TokenType.NEQ: '#',
            TokenType.LT: '<',
            TokenType.LE: '<=',
            TokenType.GT: '>',
            TokenType.GE: '>=',
            TokenType.PLUS: '+',
            TokenType.MINUS: '-',
            TokenType.TIMES: '*',
            TokenType.DIVIDE: '/',
            TokenType.LPAREN: '(',
            TokenType.RPAREN: ')',
            TokenType.COMMA: ',',
            TokenType.SEMICOLON: ';',
            TokenType.DOT: '.',
        }
        return examples.get(token_type, '')
    
    def _build_state_transition(self) -> dict:
        """构建状态转换图数据"""
        return {
            '起始': {'digit': '数字状态(1)', 'letter': '标识符状态(2)', 
                      'operator': '运算符状态(3)', 'delimiter': '结束(接受)'},
            '数字状态(1)': {'digit': '数字状态(1)', 'letter': '错误状态(4)', 
                            'other': '结束(接受)'},
            '标识符状态(2)': {'digit': '标识符状态(2)', 'letter': '标识符状态(2)', 
                              'other': '结束(接受)'},
            '运算符状态(3)': {'=': '复合运算符(5)', 'other': '结束(接受)'},
            '错误状态(4)': {'digit': '错误状态(4)', 'letter': '错误状态(4)', 
                            'other': '结束(错误)'},
            '复合运算符(5)': {'other': '结束(接受)'},
        }
    
    def peek(self, offset: int = 0) -> str:
        """查看当前字符"""
        idx = self.pos + offset
        if idx < len(self.source):
            return self.source[idx]
        return '\0'
    
    def advance(self) -> str:
        """前进一个字符"""
        ch = self.source[self.pos]
        self.pos += 1
        if ch == '\n':
            self.line += 1
            self.column = 1
        else:
            self.column += 1
        return ch
    
    def skip_whitespace(self):
        """跳过空白字符"""
        while self.pos < len(self.source) and self.peek() in ' \t\n\r':
            self.advance()
    
    def skip_line_comment(self):
        """跳过单行注释 //"""
        while self.pos < len(self.source) and self.peek() != '\n':
            self.advance()
        # 跳过换行符
        if self.pos < len(self.source):
            self.advance()
    
    def skip_block_comment(self):
        """跳过块注释 /* */"""
        start_line = self.line
        while self.pos < len(self.source) - 1:
            if self.peek() == '*' and self.peek(1) == '/':
                self.advance()  # 跳过 *
                self.advance()  # 跳过 /
                return
            self.advance()
        # 注释未闭合
        self.errors.append(Token(
            TokenType.ERROR, 
            str(start_line),
            start_line, 1,
            error_type='多行注释未闭合'
        ))
    
    def skip_whitespace_and_comments(self):
        """跳过空白和注释"""
        while self.pos < len(self.source):
            ch = self.peek()
            if ch in ' \t\n\r':
                self.advance()
            elif ch == '/' and self.peek(1) == '/':
                self.skip_line_comment()
            elif ch == '/' and self.peek(1) == '*':
                self.advance()  # 跳过 /
                self.advance()  # 跳过 *
                self.skip_block_comment()
            else:
                break
    
    def read_identifier_or_keyword(self) -> Token:
        """读取标识符或关键字"""
        start_col = self.column
        lexeme = ''
        while self.pos < len(self.source) and (self.peek().isalnum() or self.peek() == '_'):
            lexeme += self.advance()
        
        # 先检查是否是关键字（关键字不受长度限制）
        if lexeme in KEYWORDS:
            return Token(KEYWORDS[lexeme], lexeme, self.line, start_col)
        
        # 非关键字，检查标识符长度
        if len(lexeme) > 8:
            self.errors.append(Token(
                TokenType.ERROR,
                lexeme,
                self.line, start_col,
                error_type='标识符长度超长'
            ))
            return Token(TokenType.ERROR, lexeme, self.line, start_col,
                        error_type='标识符长度超长')
        
        return Token(TokenType.IDENTIFIER, lexeme, self.line, start_col)
    
    def read_number(self) -> Token:
        """读取无符号整数"""
        start_col = self.column
        lexeme = ''
        while self.pos < len(self.source) and self.peek().isdigit():
            lexeme += self.advance()
        
        # 检查长度
        if len(lexeme) > 8:
            self.errors.append(Token(
                TokenType.ERROR,
                lexeme,
                self.line, start_col,
                error_type='无符号整数越界'
            ))
            return Token(TokenType.ERROR, lexeme, self.line, start_col,
                        error_type='无符号整数越界')
        
        # 检查是否是非法单词（数字后跟字母）
        if self.pos < len(self.source) and (self.peek().isalpha() or self.peek() == '_'):
            # 这是非法单词，读取整个非法标识符
            while self.pos < len(self.source) and (self.peek().isalnum() or self.peek() == '_'):
                lexeme += self.advance()
            self.errors.append(Token(
                TokenType.ERROR,
                lexeme,
                self.line, start_col,
                error_type='非法字符(串)'
            ))
            return Token(TokenType.ERROR, lexeme, self.line, start_col,
                        error_type='非法字符(串)')
        
        # 去掉前导零的数值
        value = str(int(lexeme)) if lexeme else '0'
        return Token(TokenType.NUMBER, value, self.line, start_col)
    
    def read_operator(self) -> Token:
        """读取运算符"""
        start_col = self.column
        ch = self.advance()
        
        if ch == ':':
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.ASSIGN, ':=', self.line, start_col)
            else:
                return Token(TokenType.ERROR, ':', self.line, start_col,
                            error_type='非法字符')
        elif ch == '<':
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.LE, '<=', self.line, start_col)
            return Token(TokenType.LT, '<', self.line, start_col)
        elif ch == '>':
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.GE, '>=', self.line, start_col)
            return Token(TokenType.GT, '>', self.line, start_col)
        elif ch == '=':
            return Token(TokenType.EQ, '=', self.line, start_col)
        elif ch == '#':
            return Token(TokenType.NEQ, '#', self.line, start_col)
        elif ch == '+':
            return Token(TokenType.PLUS, '+', self.line, start_col)
        elif ch == '-':
            return Token(TokenType.MINUS, '-', self.line, start_col)
        elif ch == '*':
            return Token(TokenType.TIMES, '*', self.line, start_col)
        elif ch == '/':
            return Token(TokenType.DIVIDE, '/', self.line, start_col)
        
        return Token(TokenType.ERROR, ch, self.line, start_col,
                    error_type='非法字符')
    
    def read_delimiter(self) -> Token:
        """读取界符"""
        start_col = self.column
        ch = self.advance()
        
        delim_map = {
            '(': TokenType.LPAREN,
            ')': TokenType.RPAREN,
            ',': TokenType.COMMA,
            ';': TokenType.SEMICOLON,
            '.': TokenType.DOT,
        }
        
        if ch in delim_map:
            return Token(delim_map[ch], ch, self.line, start_col)
        return Token(TokenType.ERROR, ch, self.line, start_col,
                    error_type='非法字符')
    
    def get_next_token(self) -> Optional[Token]:
        """获取下一个词法单元"""
        self.skip_whitespace_and_comments()
        
        if self.pos >= len(self.source):
            return None
        
        ch = self.peek()
        
        # 标识符或关键字
        if ch.isalpha() or ch == '_':
            token = self.read_identifier_or_keyword()
            return token
        
        # 数字
        elif ch.isdigit():
            return self.read_number()
        
        # 运算符
        elif ch in ':=<#>+-*/':
            return self.read_operator()
        
        # 界符
        elif ch in '(),;.':
            return self.read_delimiter()
        
        # 非法字符
        else:
            start_col = self.column
            illegal = self.advance()
            self.errors.append(Token(
                TokenType.ERROR,
                illegal,
                self.line, start_col,
                error_type='非法字符'
            ))
            return Token(TokenType.ERROR, illegal, self.line, start_col,
                        error_type='非法字符')
    
    def tokenize(self) -> List[Token]:
        """对整个源程序进行词法分析"""
        self.tokens = []
        self.errors = []
        self.pos = 0
        self.line = 1
        self.column = 1
        
        while True:
            token = self.get_next_token()
            if token is None:
                break
            self.tokens.append(token)
        
        return self.tokens
    
    def print_tokens(self):
        """输出词法分析结果"""
        for token in self.tokens:
            print(str(token))
    
    def save_tokens(self, output_file: str):
        """将词法分析结果保存到文件"""
        with open(output_file, 'w', encoding='utf-8') as f:
            for token in self.tokens:
                if token.type != TokenType.ERROR:
                    f.write(token.to_file_format() + '\n')
            # 也保存错误信息
            if self.errors:
                f.write('\n# 词法错误信息:\n')
                for err in self.errors:
                    f.write(f'# {err}\n')
    
    def get_word_class_table(self) -> List[dict]:
        """获取单词分类表"""
        return self.word_class_table
    
    def get_state_transition(self) -> dict:
        """获取状态转换图数据"""
        return self.state_transition
    
    def get_analysis_flow(self) -> List[str]:
        """获取分析流程"""
        return [
            "1. 初始化：读取源程序，设置当前位置为0",
            "2. 跳过空白字符和注释",
            "3. 判断当前字符类型：",
            "   a. 字母 → 读取标识符/关键字",
            "   b. 数字 → 读取无符号整数",
            "   c. 运算符字符 → 读取运算符",
            "   d. 界符字符 → 读取界符",
            "   e. 其他 → 非法字符错误",
            "4. 输出词法单元（类型, 值）",
            "5. 重复步骤2-4直到文件结束",
        ]


# ============ 测试代码 ============

def test_correct_program():
    """测试正确的 PL/0 程序"""
    print("=" * 60)
    print("测试1: 正确的 PL/0 程序")
    print("=" * 60)
    
    source = """const a = 10;
var b, c;
procedure fun1;
if a <= 10 then
begin
    c := b + a;
end;
begin
    read(b);
    while b # 0 do
    begin
        call fun1;
        write(2 * c);
        read(b);
    end
end.
"""
    print("输入:")
    print(source)
    print("输出:")
    
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    lexer.print_tokens()
    
    if lexer.errors:
        print("\n错误:")
        for err in lexer.errors:
            print(f"  {err}")
    
    print()


def test_error_program():
    """测试含有错误的 PL/0 程序"""
    print("=" * 60)
    print("测试2: 含有错误的 PL/0 程序")
    print("=" * 60)
    
    source = """const 2a = 123456789;
var b, c;
var
//单行注释
/*
多行注释
*/
procedure function1;
if 2a <= 10 then
begin
    c := b + a;
end;
begin
    read(b);
    while b @ 0 do
    begin
        call function1;
        write(2 * c);
        read(b);
    end
end.
"""
    print("输入:")
    print(source)
    print("输出:")
    
    lexer = LexicalAnalyzer(source)
    tokens = lexer.tokenize()
    lexer.print_tokens()
    
    if lexer.errors:
        print("\n错误:")
        for err in lexer.errors:
            print(f"  {err}")
    
    print()


def test_nfa_dfa():
    """测试 NFA → DFA 转换"""
    from nfa_dfa import build_identifier_nfa, build_number_nfa, demo_automaton
    demo_automaton()


if __name__ == "__main__":
    test_correct_program()
    test_error_program()
    test_nfa_dfa()
    
    # 保存到文件示例
    source = """const a = 10;
var b, c;
procedure fun1;
if a <= 10 then
begin
    c := b + a;
end;
begin
    read(b);
    while b # 0 do
    begin
        call fun1;
        write(2 * c);
        read(b);
    end
end.
"""
    lexer = LexicalAnalyzer(source)
    lexer.tokenize()
    lexer.save_tokens('test/tokens_output.txt')
    print("\n词法分析结果已保存到 test/tokens_output.txt")
