class AVL {
private:

  struct Node {
      int val = 0;
      int h = 1; // height of subtree including this Node
      int parent = -1;
      int R = -1; // bigger numbers
      int L = -1; // less numbers
  };

    std::vector<Node> tree;
    int root = -1;
 
    int DepthDif(int pos) const {
        // left_depth - right_depth
        int left_depth = 0;
        if(tree[pos].L != -1)
            left_depth = tree[tree[pos].L].h;
        int right_depth = 0;
        if(tree[pos].R != -1)
            right_depth = tree[tree[pos].R].h;
        return left_depth - right_depth;
    }
 
    int UpdateDepth(int pos) {
        // assert that subtrees are correct
        int left_depth = 0;
        if(tree[pos].L != -1)
            left_depth = tree[tree[pos].L].h;
        int right_depth = 0;
        if(tree[pos].R != -1)
            right_depth = tree[tree[pos].R].h;
        return (tree[pos].h = std::max(left_depth, right_depth) + 1);
    }
 
    int RightRotate(int pos) {
        int son = tree[pos].L;
        assert(son != -1);
        tree[pos].L = tree[son].R;
        tree[son].R = pos;
        if(tree[pos].L != -1) {
            tree[tree[pos].L].parent = pos;
        }
        tree[son].parent = tree[pos].parent;
        tree[pos].parent = son;
 
        if (tree[son].parent == -1) {
            root = son;
        } else {
            if (tree[tree[son].parent].R == pos)
                tree[tree[son].parent].R = son;
            if (tree[tree[son].parent].L == pos)
                tree[tree[son].parent].L = son;
        }
        return son;
    }
 
    int LeftRotate(int pos) {
        int son = tree[pos].R;
        assert(son != -1);
        tree[pos].R = tree[son].L;
        tree[son].L = pos;
        if(tree[pos].R != -1) {
            tree[tree[pos].R].parent = pos;
        }
 
        tree[son].parent = tree[pos].parent;
        tree[pos].parent = son;
 
        if (tree[son].parent == -1) {
            root = son;
        } else {
            if (tree[tree[son].parent].R == pos)
                tree[tree[son].parent].R = son;
            if (tree[tree[son].parent].L == pos)
                tree[tree[son].parent].L = son;
        }
        return son;
    }
 
    void Balance(int pos) {
        // assert that subtrees are balance
        while(pos != -1) {
            int delta_H = DepthDif(pos);
            if(-1 <= delta_H && delta_H <= 1) {
                UpdateDepth(pos);
                pos = tree[pos].parent;
                continue;
            }
            if(delta_H == -2) {
                if(DepthDif(tree[pos].R) == 0 || DepthDif(tree[pos].R) == -1) {
                    LeftRotate(pos);
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    continue;
                }
 
                if(DepthDif(tree[pos].R) == 1) {
                    RightRotate(tree[pos].R);
                    LeftRotate(pos);
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    UpdateDepth(tree[pos].R);
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    continue;
                }
                assert(false);
            }
 
            if(delta_H == 2) {
                if(DepthDif(tree[pos].L) == 0 || DepthDif(tree[pos].L) == 1) {
                    RightRotate(pos);
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    continue;
                }
 
                if(DepthDif(tree[pos].L) == -1) {
                    LeftRotate(tree[pos].L);
                    RightRotate(pos);
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    UpdateDepth(tree[pos].L);
                    UpdateDepth(pos);
                    pos = tree[pos].parent;
                    continue;
                }
                assert(false);
            }
            assert(false);
        }
    }
 
    void DeleteTwoSonNode(int pos) {
        int to_swap_idx = tree[pos].R;
        while(tree[to_swap_idx].L != -1)
            to_swap_idx = tree[to_swap_idx].L;
        tree[pos].val = tree[to_swap_idx].val;
        DeleteNode(to_swap_idx);
        Balance(pos);
    }
 
    void DeleteNode(const int pos) {
        if(tree[pos].R == -1 && tree[pos].L == -1) {
            if(pos == root) {
                tree.clear();
                root = -1;
                return;
            }
            if(tree[tree[pos].parent].R == pos)
                tree[tree[pos].parent].R = -1;
            if(tree[tree[pos].parent].L == pos)
                tree[tree[pos].parent].L = -1;
            Balance(tree[pos].parent);
            return;
        }
 
        if(tree[pos].R == -1) {
            if(pos == root) {
                root = tree[pos].L;
                tree[tree[pos].L].parent = -1;
                return;
            }
            if(tree[tree[pos].parent].R == pos)
                tree[tree[pos].parent].R = tree[pos].L;
            if(tree[tree[pos].parent].L == pos)
                tree[tree[pos].parent].L = tree[pos].L;
            tree[tree[pos].L].parent = tree[pos].parent;
            Balance(tree[pos].parent);
            return;
        }
 
        if(tree[pos].L == -1) {
            if(pos == root) {
                root = tree[pos].R;
                tree[tree[pos].R].parent = -1;
                return;
            }
            if(tree[tree[pos].parent].R == pos)
                tree[tree[pos].parent].R = tree[pos].R;
            if(tree[tree[pos].parent].L == pos)
                tree[tree[pos].parent].L = tree[pos].R;
            tree[tree[pos].R].parent = tree[pos].parent;
            Balance(tree[pos].parent);
            return;
        }
        DeleteTwoSonNode(pos);
    }
 
public:
    bool Find(int key) const {
        if(tree.empty())
            return false;
        int pos = root;
        while (pos != -1) {
            if (tree[pos].val == key)
                return true;
            if (tree[pos].val < key) {
                pos = tree[pos].R;
                continue;
            }
            pos = tree[pos].L;
        }
        return false;
    }
 
    void Insert(int key) {
        if(root == -1){
            tree.emplace_back(Node());
            tree[0].val = key;
            root = 0;
            return;
        }
        int pos = root;
        while (pos != -1) {
            if (tree[pos].val == key)
                return;
            if (tree[pos].val < key) {
                if(tree[pos].R != -1) {
                    pos = tree[pos].R;
                    continue;
                }
                tree[pos].R = tree.size();
                tree.emplace_back(Node());
                tree.back().val = key;
                tree.back().parent = pos;
                Balance(pos);
                return;
            }
 
            if (tree[pos].val > key) {
                if(tree[pos].L != -1) {
                    pos = tree[pos].L;
                    continue;
                }
                tree[pos].L = tree.size();
                tree.emplace_back(Node());
                tree.back().val = key;
                tree.back().parent = pos;
                Balance(pos);
                return;
            }
        }
    }
 
    void Erase(int key) {
        if(tree.empty())
            return;
        int pos = root;
        while (pos != -1) {
            if (tree[pos].val == key) {
                DeleteNode(pos);
                return;
            }
            if (tree[pos].val < key) {
                if(tree[pos].R != -1) {
                    pos = tree[pos].R;
                    continue;
                }
                return;
            }
            if (tree[pos].val > key) {
                if(tree[pos].L != -1) {
                    pos = tree[pos].L;
                    continue;
                }
                return;
            }
        }
    }
 
    int NextElement(int key) const {
        if(tree.empty())
            return INF;
        int ans = INF;
        int pos = root;
        while (pos != -1) {
            if(tree[pos].val > key) {
                ans = tree[pos].val;
                pos = tree[pos].L;
                continue;
            }
            pos = tree[pos].R;
        }
        return ans;
    }
 
    int PrevElement(int key) const {
        if(tree.empty())
            return -INF;
        int ans = -INF;
        int pos = root;
        while (pos != -1) {
            if(tree[pos].val < key) {
                ans = tree[pos].val;
                pos = tree[pos].R;
                continue;
            }
            pos = tree[pos].L;
        }
        return ans;
    }
};
