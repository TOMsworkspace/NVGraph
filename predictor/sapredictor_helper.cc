//
// Created by tom on 2020/10/16.
//
#include<string>
#include<iostream>
#include "sapredictor_helper.h"
#include "base_cache.h"

using namespace std;

void Bimodal::fastTransferA(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::fastTransferB(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::True_unfair_TransferA(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakmiss;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::True_unfair_TransferB(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakmiss;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::False_unfair_TransferA(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::False_unfair_TransferB(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::slowTransferA(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakmiss;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::slowTransferB(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakmiss;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::slowTransfer(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakmiss;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::fastTransfer(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakmiss;
        else
            curstate = strongmiss;
        break;
    }
    }
}

void Bimodal::unfairTransfer(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakhit;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    }
}

// 改进
void Bimodal::adaptiveTransfer(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;

        break;
    }
    case state::weakhit:
    {
        if (action == Hit)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;

        break;
    }
    case state::strongmiss:
    {
        if (action == Hit)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    }
}

// 应对随机序列的问题
// 下一状态只与当前状态有关
void Bimodal::randTransferA(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {
        if (action == Miss)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakhit:
    {
        if (action == Miss)
            curstate = stronghit;
        else
            curstate = weakmiss;
        break;
    }
    case state::weakmiss:
    {
        if (action == Miss)
            curstate = weakhit;
        else
            curstate = strongmiss;
        break;
    }
    case state::strongmiss:
    {
        if (action == Miss)
            curstate = weakhit;
        else
            curstate = stronghit;
        break;
    }
    }
}
/*
    void Bimodal::randTransferA(const cacheAction action){
        switch (curstate) {
            case state::stronghit :{

                curstate=strongmiss;
                break;
            }
            case state::weakhit:{

                curstate=weakmiss;
                break;
            }
            case state::weakmiss:{

                curstate=weakhit;
                break;
            }
            case state::strongmiss:{

                curstate=stronghit;
                break;
            }
        }
    }
*/
void Bimodal::randTransferB(const cacheAction action)
{
    switch (curstate)
    {
    case state::stronghit:
    {

        curstate = weakmiss;
        break;
    }
    case state::weakhit:
    {

        curstate = strongmiss;
        break;
    }
    case state::weakmiss:
    {

        curstate = stronghit;
        break;
    }
    case state::strongmiss:
    {

        curstate = weakhit;
        break;
    }
    }
}

Bimodal::Bimodal(enum stateTransfer transfer, enum state state) : transfer(transfer), curstate(state)
{
}

bool Bimodal::predict(const operation &op)
{ // 预测
    //   if(op==OPERATION_WRITE)
    //       return false;

    switch (curstate)
    {
    case stronghit:
    case weakhit:
        return true;
    case weakmiss:
    case strongmiss:
        return false;
    }

    return false;
}

bool Bimodal::stateTransform(const cacheAction action)
{
    switch (transfer)
    {
        case stateTransfer::fast:
        {
            fastTransfer(action);
            break;
        }
        case stateTransfer::slow:
        {
            slowTransfer(action);
            break;
        }
        case stateTransfer::unfair:
        {
            unfairTransfer(action);
            break;
        }
        case stateTransfer::fastA:
        {
            fastTransferA(action);
            break;
        }
        case stateTransfer::fastB:
        {
            fastTransferB(action);
            break;
        }
        case stateTransfer::true_unfairA:
        {
            True_unfair_TransferA(action);
            break;
        }
        case stateTransfer::true_unfairB:
        {
            True_unfair_TransferB(action);
            break;
        }
        case stateTransfer::false_unfairA:
        {
            False_unfair_TransferA(action);
            break;
        }
        case stateTransfer::false_unfairB:
        {
            False_unfair_TransferB(action);
            break;
        }
        case stateTransfer::slowA:
        {
            slowTransferA(action);
            break;
        }
        case stateTransfer::slowB:
        {
            slowTransferB(action);
            break;
        }
        case stateTransfer::rand_transferA:
        {
            randTransferA(action);
            break;
        }
        case stateTransfer::rand_transferB:
        {
            randTransferB(action);
            break;
        }
    }
    return true;
}

void Bimodal::setState(enum state curstate)
{
    this->curstate = curstate;
}

state Bimodal::getState() const
{
    return this->curstate;
}

void Bimodal::setTransfer(enum stateTransfer transfer)
{
    this->transfer = transfer;
}

enum stateTransfer Bimodal::getTransfer() const
{
    return this->transfer;
}

Trie::Trie()
{
    root = new Trienode();
}
// 析构函数
Trie::~Trie()
{
    for (int i = 0; i < 26; i++)
        if (this->root->next[i])
            deleteNode(this->root->next[i]);

    delete this->root;
    this->root = nullptr;
}

// Trie中的查找操作
Trienode *Trie::search(string key) const
{
    Trienode *p = root;
    int l = key.size();
    for (int i = 0; i < l; ++i)
    {
        if (p->next[key[i] - 'A'] == nullptr)
        {
            return nullptr;
        }
        p = p->next[key[i] - 'A'];
    }
    return p;
}

// Trie中的插入操作 private
Trienode *Trie::insert(string key, const enum stateTransfer transfer)
{
    Trienode *p = this->root;

    int l = key.size();
    for (int i = 0; i < l; ++i)
    {
        if (p->next[key[i] - 'A'] == nullptr)
        {
            Trienode *tmp = new Trienode;
            tmp->transfer = transfer;
            p->next[key[i] - 'A'] = tmp;
        }

        p = p->next[key[i] - 'A'];
        p->count++;
    }

    this->root->count++;

    return p;
}

// 清理字典
void Trie::clear()
{
    for (int i = 0; i < 26; i++)
        if (this->root->next[i])
            deleteNode(root->next[i]);
}

void Trie::deleteNode(Trienode *root)
{
    if (root == nullptr)
        return;

    for (int i = 0; i < 26; ++i)
    {
        if (root->next[i])
        {
            deleteNode(root->next[i]);
        }
    }

    delete root;
    this->root->count--;
    root = nullptr;
}

void Trie::deleteNode(string key)
{

    Trienode *p = root;
    if (root == nullptr)
        return;

    int l = key.size();
    for (int i = 0; i <= l - 1; ++i)
    {
        if (p->next[key[i] - 'A'] == nullptr)
        {
            return;
        }
        p = p->next[key[i] - 'A'];
        p->count--;
    }

    if (p->count == 0)
        deleteNode(p);
    p = nullptr;
}

bool Trie::isleaf(const Trienode *root) const
{
    if (root == nullptr)
        return false;

    for (int i = 0; i < 26; ++i)
    {
        if (root->next[i])
        {
            return false;
        }
    }

    return true;
}

void Trie::print(const Trienode *root, ostream &outfile, string str)
{
    if (root == nullptr)
        return;

    if (str.size() > 2 && str.size() <= SEQOUT)
    {
        outfile << "sequence: " << str << " count: " << root->count << endl;
    }

    for (int i = 0; i < 26; ++i)
    {
        if (root->next[i])
        {
            print(root->next[i], outfile, str + char('A' + i));
        }
    }
}

ostream &operator<<(ostream &os, Trie &trie)
{

    trie.print(trie.root, os);

    return os;
}
