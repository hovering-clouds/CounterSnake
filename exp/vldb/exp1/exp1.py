alg = ["bm", "bs", "diamond", "dway", "pyramid", "sac", "stingy"]

for name in alg:
    fname = f"csize-{name}.txt"
    result = [0.0]*24
    num = [0]*24
    with open(fname, "r") as fin:
        for line in fin.readlines():
            pr = line.split(" ")
            sz1 = float(pr[0])
            sz2 = int(pr[1])
            num[sz2]+=1
            result[sz2]+=sz1
        for i in range(24):
            if num[i]!=0:
                result[i]/=num[i]
    print(name, end=" ")
    for avg in result:
        print(f"{avg:.2f}", end=" ")
    print("")
