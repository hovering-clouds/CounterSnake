import math

digit_cnt = [0]*32

def get_cnt(fname):
    with open(fname) as fin:
        for line in fin.readlines():
            for num_str in line.split():
                num = max(int(num_str),1)
                digit = math.floor(math.log2(num))
                digit_cnt[digit]+=1

def show_result1():
    # print(digit_cnt)
    result = [0,0,0,0]
    num_sum = sum(digit_cnt)
    for i in range(4):
        result[i] = sum(digit_cnt[4*i:])/num_sum
    result.append(sum(digit_cnt[16:])/num_sum)
    print(f"sum={num_sum}, result={result}")

def show_result2():
    num_sum = sum(digit_cnt)
    result = [0]*16
    for i in range(16):
        result[i] = math.log10(sum(digit_cnt[i:])/num_sum)
    print(f"sum={num_sum}, result={result}")

def show_result3():
    cnt_sum = sum(digit_cnt)
    bitsum = 0
    for i in range(16):
        bitsum+=(1+i)*digit_cnt[i]
    print(f"fixed={cnt_sum*16/8/1024}, var={bitsum/8/1024}, reduct={(cnt_sum*16-bitsum)/cnt_sum/16}")
   

#for prefix in ["cm", "fr", "es", "pr", "dt", "hp", "sl", "mv"]:
#    f_name = f"./exp1-cnt-{prefix}.txt"
#    get_cnt(f_name)
get_cnt("exp1-cnt-hp.txt")
show_result3()
