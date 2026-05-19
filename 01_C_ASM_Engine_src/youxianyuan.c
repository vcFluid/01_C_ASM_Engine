#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


// 定义邻居节点结构（链表）
typedef struct Node {
    int column;
    struct Node* next;
} Node;




// 向邻接表添加节点（保持升序，便于找 j_min）
void add_neighbor(Node** head, int col) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->column = col;
    new_node->next = NULL;

    if (*head == NULL || (*head)->column > col) {
        new_node->next = *head;
        *head = new_node;
        return;
    }

    Node* curr = *head;
    while (curr->next != NULL && curr->next->column < col) {
        if (curr->next->column == col) { // 已存在，去重
            free(new_node);
            return;
        }
        curr = curr->next;
    }
    
    if (curr->column == col) { // 再次检查当前节点
        free(new_node);
        return;
    }

    new_node->next = curr->next;
    curr->next = new_node;
}

int main() {
    int n = 16; // 16x16 矩阵
    Node* adj[17] = {NULL}; // 1-indexed 邻接表
    bool is_original[17][17] = {false}; // 原始邻居指纹表（红块）

    // 1. 定义原始边列表（每行代表一条物理连接）
    int raw_edges[][2] = {
        // --- 内圈连接 ---
        {1, 2}, {2, 4}, {4, 6}, {6, 8}, {7, 8}, {5, 7}, {3, 5}, {1, 3},
        // --- 外圈连接 ---
        {9, 10}, {10, 12}, {12, 14}, {14, 16}, {15, 16}, {13, 15}, {11, 13}, {9, 11},
        // --- 径向连接（内外圈对应） ---
        {1, 9}, {2, 10}, {3, 11}, {4, 12}, {5, 13}, {6, 14}, {7, 15}, {8, 16}
    };

    int num_edges = sizeof(raw_edges) / sizeof(raw_edges[0]);

    // 2. 循环将边输入邻接表和原始矩阵
    for (int e = 0; e < num_edges; e++) {
        int u = raw_edges[e][0];
        int v = raw_edges[e][1];

        // 规范化：确保 i < j，只存储上三角
        int i = (u < v) ? u : v;
        int j = (u > v) ? u : v;

        add_neighbor(&adj[i], j);
        is_original[i][j] = true;
    }

    printf("开始符号分解递推...\n");
    int fill_in_count = 0;

    // 2. 核心递推：Gravity 消元逻辑
    for (int i = 1; i <= n; i++) {
        if (adj[i] == NULL) continue;

        // 第一个邻居即为 j_min（接力手）
        int j_min = adj[i]->column;
        Node* others = adj[i]->next;

        // 将 i 的剩余债务接力给 j_min
        while (others != NULL) {
            int k = others->column;

            // 判据：如果 (j_min, k) 不是原始边，且目前在 j_min 行是白块
            // 我们先尝试添加，add_neighbor 函数会自动处理“已存在”的情况
            
            // 在 C 中，我们直接看是否为新坑
            bool already_exists = false;
            Node* temp = adj[j_min];
            while(temp != NULL) {
                if(temp->column == k) { already_exists = true; break; }
                temp = temp->next;
            }

            if (!already_exists && !is_original[j_min][k] && j_min < k) {
                printf("发现新填充（蓝块）: (%d, %d)\n", j_min, k);
                fill_in_count++;
            }

            // 执行“塌缩”：无论是否为填充，信息都要传给 j_min
            if (j_min < k) {
                add_neighbor(&adj[j_min], k);
            }
            others = others->next;
        }
    }

    printf("--------------------------\n");
    printf("检测完毕。总填充数（蓝块）: %d\n", fill_in_count);

    // 释放内存（良好的 C 习惯）
    for (int i = 1; i <= n; i++) {
        Node* curr = adj[i];
        while (curr) {
            Node* temp = curr;
            curr = curr->next;
            free(temp);
        }
    }

    system("pause");
    return 0;
}