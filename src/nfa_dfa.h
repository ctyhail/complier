/**
 * nfa_dfa.h - NFA→DFA→最小化接口
 * 演示正规式到NFA到DFA到最小DFA的完整过程
 */
#ifndef NFA_DFA_H
#define NFA_DFA_H

#define MAX_STATES 128
#define MAX_EDGE 16

/* ========== NFA节点 ========== */
typedef struct NFANode_ {
    int id;
    struct {
        char input;           /* 'ε', 'L'(letter), 'D'(digit), 或具体字符 */
        struct NFANode_ *dest;
    } edges[MAX_EDGE];
    int edge_count;
    int is_accept;           /* 是否为接受状态 */
} NFANode;

/* ========== NFA ========== */
typedef struct {
    NFANode *start;
} NFA;

/* ========== NFA状态集合 ========== */
typedef struct {
    int states[MAX_STATES];
    int count;
} NfaSet;

/* ========== DFA状态 ========== */
typedef struct {
    int id;
    int is_accept;
    NfaSet nfa_states;       /* 对应的NFA状态集 */
    struct {
        char input;
        int dest;
    } trans[MAX_EDGE];
    int trans_count;
} DFAState;

/* 全局状态计数器 */
static int nfa_state_counter = 0;

/* 集合操作 */
static inline void set_init(NfaSet *s)
{
    memset(s->states, -1, sizeof(s->states));
    s->count = 0;
}

static inline void set_add(NfaSet *s, int state)
{
    for (int i = 0; i < s->count; i++)
        if (s->states[i] == state) return;
    s->states[s->count++] = state;
}

static inline int set_contains(NfaSet *s, int state)
{
    for (int i = 0; i < s->count; i++)
        if (s->states[i] == state) return 1;
    return 0;
}

/* 构造NFA示例 */
NFA nfa_construct_ident(void);
NFA nfa_construct_number(void);

/* ε闭包和move操作 */
NfaSet epsilon_closure(NfaSet *s);
NfaSet move_set(NfaSet *s, char c);

/* DFA最小化 */
DFAState dfa_minimize(DFAState *states, int state_count);

/* 演示输出 */
void nfa_dfa_demo(void);

#endif /* NFA_DFA_H */
