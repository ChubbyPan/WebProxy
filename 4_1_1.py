def Attack_revenue(P, a, n, V):
    ans, t = 0, len(P[0])
    for j in range(t):
        ans += V[j] * ptotal(P,a,n,j)
    return ans

def ptotal(P, a, n, j: int):
    total, m = 1.0, len(P)
    for i in range(m):
        total *=  (1 - a[i][j] * P[i][j]) ** n[i][j]
    return 1 - total

P = [[0.5,0.5],[0.5,0.5]]
a = [[1,1],[1,1]]
n = [[1,1],[1,1]]
V = [1,1]
# 参数： P：矩阵，a：矩阵，n：矩阵，V：数组, m行t列
# print(ptotal(P,a,n,0))
# print(Attack_revenue(P=P,a=a,n=n,V=V))
    