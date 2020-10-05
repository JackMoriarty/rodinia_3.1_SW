import csv
import matplotlib.pyplot as plt

class Vividict(dict):
    def __missing__(self, key):
        value = self[key] = type(self)() # retain local pointer to value
        return value                     # faster to return than dict lookup

def get_data(filename):
    # 计算相应项目时间的总和
    sum = Vividict()
    with open(filename, 'r') as f:
        lines = f.readlines()
        for line in lines:
            substr = line.split(':')
            if substr[0] not in sum or substr[1] not in sum[substr[0]] :
                sum[substr[0]][substr[1]] = float(substr[2])
            else:
                sum[substr[0]][substr[1]] += float(substr[2])
    return sum

benchmark_name = ("backprop", "bfs", "hotspot3D", "kmeans", "pathfinder", "nw")
version_name = ("openACC", "Athread")

if __name__ == '__main__':
    average_value = get_data('result.txt')

    speedup = []
    labels = []
    for benchmark_item in benchmark_name:
        for version_item in version_name:
            speedup_item = round(average_value[benchmark_item]['Serial'] / average_value[benchmark_item][version_item], 2)
            speedup.append(speedup_item)
            labels.append(benchmark_item+"-"+version_item)

    # 绘制柱状图
    plt.figure(figsize=(10, 4))
    plt.bar(labels, speedup)
    plt.xticks(labels, labels, rotation=60, fontsize=16)
    plt.yticks(fontsize=10)
    plt.ylabel("speedup", fontsize=16)
    for a, b in zip(labels, speedup):
        plt.text(a, b + 0.05, '%.2fx' % b, ha='center', va='bottom', fontsize=16)
        
    #拉伸y轴
    plt.ylim(0,45)

    plt.tight_layout()

    # plt.savefig('result.png', dpi=300)
    plt.savefig('result.pdf', format='PDF')
    plt.show()
