int main() {
    int sum = 0;
    for (int i = 1; i <= 10; i = i + 1) {
        sum = sum + i;
    }
    
    int j = 10;
    while (j > 0) {
        sum = sum - 1;
        j = j - 1;
    }
    
    if (sum == 45) {
        return 0; // Success: 55 - 10 = 45
    } else {
        return sum;
    }
}
