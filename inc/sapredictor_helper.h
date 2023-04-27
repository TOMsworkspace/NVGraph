//
// Created by tom on 2020/10/16.
//

#ifndef SAPREDICTOR_HELPER_H
#define SAPREDICTOR_HELPER_H

#include<string>
#include<iostream>
#include "base_cache.h"
using namespace std;

#define SEQOUT 10

enum state
{
    stronghit = 11,
    weakhit = 10,
    weakmiss = 01,
    strongmiss = 00
};
enum stateTransfer
{
    slow = 0,
    fast = 1,
    unfair = 2,
    fastA = 3,
    fastB = 4,
    true_unfairA = 5,
    true_unfairB = 6,
    false_unfairA = 7,
    false_unfairB = 8,
    slowA = 9,
    slowB = 10,
    adaptive = 11,
    rand_transferA = 12,
    rand_transferB = 13
};

class StateMachine
{
private:
public:
    StateMachine(){};
    virtual ~StateMachine(){};
    virtual bool stateTransform(const cacheAction action) = 0;
};

class Bimodal : public StateMachine
{

private:

    enum stateTransfer transfer;
    enum state curstate;
    void fastTransferA(const cacheAction action);
    void fastTransferB(const cacheAction action);
    void True_unfair_TransferA(const cacheAction action);
    void True_unfair_TransferB(const cacheAction action);
    void False_unfair_TransferA(const cacheAction action);
    void False_unfair_TransferB(const cacheAction action);
    void slowTransferA(const cacheAction action);
    void slowTransferB(const cacheAction action);
    void slowTransfer(const cacheAction action);
    void fastTransfer(const cacheAction action);
    void unfairTransfer(const cacheAction action);
    // 改进
    void adaptiveTransfer(const cacheAction action);
    // 应对随机序列的问题
    // 下一状态只与当前状态有关
    void randTransferA(const cacheAction action);
    void randTransferB(const cacheAction action);

public:
    Bimodal(enum stateTransfer transfer = slow, enum state state = strongmiss);
    bool predict(const operation &op);
    bool stateTransform(const cacheAction action) override;
    void setState(enum state curstate);
    state getState() const;
    void setTransfer(enum stateTransfer transfer);
    enum stateTransfer getTransfer() const;
};

struct Trienode
{
    int count;

    enum stateTransfer transfer;

    Trienode *next[26];

    Trienode()
    {
        count = 0;

        transfer = slowA;

        for (int i = 0; i < 26; ++i)
            next[i] = nullptr;
    }
};

class Trie
{
public:
    Trie();

    ~Trie();

    void clear();

    Trienode *search(string key) const;
    Trienode *insert(string key, const enum stateTransfer transfer);
    void deleteNode(string key);
    friend ostream &operator<<(ostream &out, Trie &trie);

private:
    void print(const Trienode *root, ostream &out, string str = "");
    void deleteNode(Trienode *root);

    Trienode *root;

    bool isleaf(const Trienode *root) const;
};

#endif // SAPREDICTOR_HELPER_H
