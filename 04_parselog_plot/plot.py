# plot_temp.py
# 读取 out.csv (time,soc_pcb)，按时间绘制温度变化曲线
# 运行：python plot_temp.py out.csv

import sys
import pandas as pd
import matplotlib.pyplot as plt

def main(csv_path: str):
    df = pd.read_csv(csv_path)

    t = pd.to_datetime(df["time"], format="%H:%M:%S", errors="coerce")
    df = df[t.notna()].copy()
    t = t[t.notna()]
    df["sec"] = (t.dt.hour * 3600 + t.dt.minute * 60 + t.dt.second).astype(int)

    df["soc_pcb"] = pd.to_numeric(df["soc_pcb"], errors="coerce")

    # 过滤掉 -100 的无效点
    df = df[df["soc_pcb"] != -100]

    df = df.dropna(subset=["soc_pcb", "sec"]).sort_values("sec")

    x = df["sec"].to_numpy()
    y = df["soc_pcb"].to_numpy()

    plt.figure(figsize=(12, 4.5))
    plt.plot(x, y, linewidth=1.5)
    plt.title("SOC_PCB Temperature vs Time")
    plt.xlabel("Time (HH:MM:SS)")
    plt.ylabel("Temperature")
    plt.grid(True, linestyle="--", alpha=0.4)

    step = max(len(df) // 10, 1)
    xt = df.iloc[::step]
    plt.xticks(xt["sec"].to_numpy(), xt["time"].to_numpy(), rotation=45, ha="right")

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} out.csv")
        sys.exit(1)
    main(sys.argv[1])