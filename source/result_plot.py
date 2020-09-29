import csv
import matplotlib.pyplot as plt

class Vividict(dict):
    def __missing__(self, key):
        value = self[key] = type(self)() # retain local pointer to value
        return value                     # faster to return than dict lookup

def get_data(filename):
    # 计算相应项目的平均值
    average_value = Vividict()
    with open(filename, 'r') as csvfile:
        reader = csv.reader(csvfile)
        
        for item in reader:
            if reader.line_num == 1:
                continue
            sum = 0
            for i in range(2, 7):
                sum += float(item[i])
            sum /= 5
            average_value[item[0]][item[1]] = sum
    
    return average_value

single_average_value = {
        "backprop":106760.6,
        "bfs":918989.6,
        "hotspot3D":6598800.8,
        "kmeans":8523158.8,
        "pathfinder":127370.0,
        "nw":166362
    }

benchmark_name = ("backprop", "bfs", "hotspot3D", "kmeans", "pathfinder", "nw")
version_name = ("openACC", "Athread")

if __name__ == '__main__':
    average_value = get_data('result.csv')

    speedup = []
    labels = []
    for benchmark_item in benchmark_name:
        for version_item in version_name:
            speedup_item = round(single_average_value[benchmark_item] / average_value[benchmark_item][version_item], 2)
            speedup.append(speedup_item)
            labels.append(benchmark_item+"/"+version_item)

# 绘制柱状图
plt.figure(figsize=(10, 4))
plt.bar(labels, speedup)
plt.xticks(labels, labels, rotation=60, fontsize=8)
plt.ylabel("speedup")
for a, b in zip(labels, speedup):
    plt.text(a, b + 0.05, '%.2fx' % b, ha='center', va='bottom', fontsize=8)

plt.tight_layout()

# plt.savefig('result.png', dpi=300)
plt.savefig('result.pdf', format='PDF')
plt.show()
