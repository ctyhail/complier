"""
nfa_dfa.py
正规式 → NFA → DFA → DFA 最小化 的完整实现
用于编译原理课程设计词法分析部分的自动机演示
"""

from typing import Dict, List, Set, Tuple, Optional
from collections import defaultdict
import json


class NFAState:
    """NFA 状态"""
    def __init__(self, state_id: int):
        self.id = state_id
        # 转移: 字符 -> 目标状态集合
        # None 表示 ε (epsilon) 转移
        self.transitions: Dict[Optional[str], Set[int]] = defaultdict(set)
        self.is_accept = False
        self.accept_token = None  # 接受的 token 类型

    def add_transition(self, char: Optional[str], target_id: int):
        self.transitions[char].add(target_id)

    def __repr__(self):
        return f"NFAState({self.id}, accept={self.is_accept})"


class NFA:
    """NFA - 非确定有限自动机"""
    def __init__(self):
        self.states: List[NFAState] = []
        self.start_state = 0
        self.alphabet: Set[str] = set()

    def new_state(self) -> int:
        sid = len(self.states)
        self.states.append(NFAState(sid))
        return sid

    def add_transition(self, from_id: int, char: Optional[str], to_id: int):
        if char is not None:
            self.alphabet.add(char)
        self.states[from_id].add_transition(char, to_id)

    def epsilon_closure(self, states: Set[int]) -> Set[int]:
        """计算 ε-闭包"""
        closure = set(states)
        stack = list(states)
        while stack:
            s = stack.pop()
            for next_s in self.states[s].transitions.get(None, set()):
                if next_s not in closure:
                    closure.add(next_s)
                    stack.append(next_s)
        return closure

    def move(self, states: Set[int], char: str) -> Set[int]:
        """从状态集合经过一个字符的转移"""
        result = set()
        for s in states:
            for next_s in self.states[s].transitions.get(char, set()):
                result.add(next_s)
        return result

    def to_dfa(self) -> 'DFA':
        """NFA → DFA 转换 (子集构造法)"""
        dfa = DFA()
        start_closure = self.epsilon_closure({self.start_state})
        dfa_state_map: Dict[frozenset, int] = {}
        
        # DFA 状态队列
        unmarked = []
        
        dfa_start = dfa.new_state()
        dfa_state_map[frozenset(start_closure)] = dfa_start
        unmarked.append(start_closure)
        
        # 标记起始状态是否为接受状态
        if any(self.states[s].is_accept for s in start_closure):
            dfa.states[dfa_start].is_accept = True
            for s in start_closure:
                if self.states[s].is_accept and self.states[s].accept_token:
                    dfa.states[dfa_start].accept_token = self.states[s].accept_token
                    break

        while unmarked:
            nfa_states = unmarked.pop(0)
            dfa_sid = dfa_state_map[frozenset(nfa_states)]
            
            for char in sorted(self.alphabet):
                move_set = self.move(nfa_states, char)
                if not move_set:
                    continue
                
                closure = self.epsilon_closure(move_set)
                key = frozenset(closure)
                
                if key not in dfa_state_map:
                    new_sid = dfa.new_state()
                    dfa_state_map[key] = new_sid
                    unmarked.append(closure)
                    
                    # 检查是否为接受状态
                    if any(self.states[s].is_accept for s in closure):
                        dfa.states[new_sid].is_accept = True
                        for s in closure:
                            if self.states[s].is_accept and self.states[s].accept_token:
                                dfa.states[new_sid].accept_token = self.states[s].accept_token
                                break
                
                target_sid = dfa_state_map[key]
                dfa.add_transition(dfa_sid, char, target_sid)
        
        return dfa


class DFAState:
    """DFA 状态"""
    def __init__(self, state_id: int):
        self.id = state_id
        self.transitions: Dict[str, int] = {}
        self.is_accept = False
        self.accept_token = None

    def __repr__(self):
        return f"DFAState({self.id}, accept={self.is_accept})"


class DFA:
    """DFA - 确定有限自动机"""
    def __init__(self):
        self.states: List[DFAState] = []
        self.start_state = 0
        self.alphabet: Set[str] = set()

    def new_state(self) -> int:
        sid = len(self.states)
        self.states.append(DFAState(sid))
        return sid

    def add_transition(self, from_id: int, char: str, to_id: int):
        self.alphabet.add(char)
        self.states[from_id].transitions[char] = to_id

    def minimize(self) -> 'DFA':
        """DFA 最小化 - Hopcroft 算法"""
        n = len(self.states)
        if n == 0:
            return self

        # 初始划分: 接受状态和非接受状态
        accept_states = {s.id for s in self.states if s.is_accept}
        non_accept_states = {s.id for s in self.states if not s.is_accept}
        
        partitions = []
        if non_accept_states:
            partitions.append(non_accept_states)
        if accept_states:
            partitions.append(accept_states)

        # 迭代细化
        changed = True
        while changed:
            changed = False
            new_partitions = []
            
            for part in partitions:
                if len(part) <= 1:
                    new_partitions.append(part)
                    continue
                
                # 根据每个字符的转移目标来分裂
                split_map: Dict[tuple, set] = {}
                for state_id in part:
                    sig = []
                    for ch in sorted(self.alphabet):
                        target = self.states[state_id].transitions.get(ch, -1)
                        # 找到目标状态所属的分区索引
                        part_idx = -2
                        for i, p in enumerate(partitions):
                            if target in p:
                                part_idx = i
                                break
                        if target == -1:
                            part_idx = -1
                        sig.append(part_idx)
                    key = tuple(sig)
                    if key not in split_map:
                        split_map[key] = set()
                    split_map[key].add(state_id)
                
                if len(split_map) > 1:
                    changed = True
                    for s in split_map.values():
                        new_partitions.append(s)
                else:
                    new_partitions.append(part)
            
            partitions = new_partitions

        # 构建最小化 DFA
        min_dfa = DFA()
        state_map: Dict[int, int] = {}  # 旧状态 -> 新状态
        
        for part in partitions:
            rep = min(part)
            new_sid = min_dfa.new_state()
            state_map[rep] = new_sid
            min_dfa.states[new_sid].is_accept = self.states[rep].is_accept
            min_dfa.states[new_sid].accept_token = self.states[rep].accept_token
        
        # 设置起始状态
        for old_s, new_s in state_map.items():
            if old_s == self.start_state:
                min_dfa.start_state = new_s
                break
        
        # 添加转移
        for old_s, new_s in state_map.items():
            for ch, target in self.states[old_s].transitions.items():
                # 找到 target 所在分区的代表
                for part in partitions:
                    if target in part:
                        rep = min(part)
                        min_dfa.add_transition(new_s, ch, state_map[rep])
                        break
        
        min_dfa.alphabet = self.alphabet
        return min_dfa

    def simulate(self, input_str: str) -> Tuple[bool, Optional[str]]:
        """模拟 DFA 运行"""
        current = self.start_state
        for ch in input_str:
            if ch not in self.states[current].transitions:
                return False, None
            current = self.states[current].transitions[ch]
        
        if self.states[current].is_accept:
            return True, self.states[current].accept_token
        return False, None

    def get_transition_table(self) -> List[Dict]:
        """获取 DFA 转移表（用于可视化）"""
        table = []
        for s in self.states:
            row = {
                'state': s.id,
                'is_accept': s.is_accept,
                'accept_token': s.accept_token,
                'transitions': dict(sorted(s.transitions.items()))
            }
            table.append(row)
        return table


def build_identifier_nfa() -> NFA:
    """构建标识符的 NFA: letter(letter|digit)*"""
    nfa = NFA()
    s0 = nfa.new_state()  # 起始状态
    s1 = nfa.new_state()  # 接受状态
    
    nfa.start_state = s0
    nfa.states[s1].is_accept = True
    nfa.states[s1].accept_token = '标识符'
    
    # letter 转移 (a-z, A-Z)
    for c in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_':
        nfa.add_transition(s0, c, s1)
        nfa.add_transition(s1, c, s1)
    
    # digit 转移 (在 s1 上继续)
    for c in '0123456789':
        nfa.add_transition(s1, c, s1)
    
    return nfa


def build_number_nfa() -> NFA:
    """构建无符号整数的 NFA: digit+"""
    nfa = NFA()
    s0 = nfa.new_state()
    s1 = nfa.new_state()
    
    nfa.start_state = s0
    nfa.states[s1].is_accept = True
    nfa.states[s1].accept_token = '无符号整数'
    
    for c in '0123456789':
        nfa.add_transition(s0, c, s1)
        nfa.add_transition(s1, c, s1)
    
    return nfa


def demo_automaton():
    """演示 NFA → DFA → 最小化 DFA 的完整过程"""
    print("=" * 60)
    print("正规式 → NFA → DFA → DFA 最小化 演示")
    print("=" * 60)
    
    # 标识符: letter(letter|digit)*
    print("\n1. 标识符正规式: letter(letter|digit)*")
    nfa_id = build_identifier_nfa()
    print(f"   NFA 状态数: {len(nfa_id.states)}")
    print(f"   NFA 字母表: {''.join(sorted(nfa_id.alphabet))}")
    
    dfa_id = nfa_id.to_dfa()
    print(f"   DFA 状态数: {len(dfa_id.states)}")
    
    min_dfa_id = dfa_id.minimize()
    print(f"   最小化 DFA 状态数: {len(min_dfa_id.states)}")
    
    # 测试
    for test in ["abc", "x1", "a", "var_name", "123abc"]:
        accepted, token = min_dfa_id.simulate(test)
        print(f"   '{test}' -> {'接受(' + str(token) + ')' if accepted else '拒绝'}")
    
    # 无符号整数: digit+
    print("\n2. 无符号整数正规式: digit+")
    nfa_num = build_number_nfa()
    dfa_num = nfa_num.to_dfa()
    min_dfa_num = dfa_num.minimize()
    print(f"   NFA 状态数: {len(nfa_num.states)}")
    print(f"   DFA 状态数: {len(dfa_num.states)}")
    print(f"   最小化 DFA 状态数: {len(min_dfa_num.states)}")
    
    for test in ["123", "0", "007", "abc"]:
        accepted, token = min_dfa_num.simulate(test)
        print(f"   '{test}' -> {'接受(' + str(token) + ')' if accepted else '拒绝'}")


if __name__ == "__main__":
    demo_automaton()
