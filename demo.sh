#!/bin/bash
#
# Script demo cho Driver USB Caesar Cipher
# Sử dụng: chmod +x demo.sh && ./demo.sh
#

DEVICE="/dev/crypto0"

# Màu sắc
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== Demo Driver USB Caesar Cipher ===${NC}"

# Kiểm tra device file
if [[ ! -e "$DEVICE" ]]; then
    echo -e "${RED}Lỗi: Device file $DEVICE không tồn tại${NC}"
    echo "Kiểm tra:"
    echo "1. Driver đã được nạp chưa: lsmod | grep usb_crypto"
    echo "2. Thiết bị USB đã kết nối chưa: lsusb | grep 0951:1666"
    exit 1
fi

echo -e "${GREEN}Device $DEVICE sẵn sàng${NC}"
echo

# Demo mã hóa
echo -e "${BLUE}Demo 1: Mã hóa văn bản${NC}"
TEXT="Hello World"
echo "Văn bản gốc: $TEXT"
echo -n "E:$TEXT" > $DEVICE
ENCODED=$(cat $DEVICE)
echo "Kết quả mã hóa: $ENCODED"
echo

# Demo giải mã
echo -e "${BLUE}Demo 2: Giải mã văn bản${NC}"
echo "Văn bản mã hóa: $ENCODED"
echo -n "D:$ENCODED" > $DEVICE
DECODED=$(cat $DEVICE)
echo "Kết quả giải mã: $DECODED"
echo

# Kiểm tra tính đúng đắn
if [[ "$TEXT" == "$DECODED" ]]; then
    echo -e "${GREEN}✓ Test PASS: Văn bản gốc và giải mã trùng khớp${NC}"
else
    echo -e "${RED}✗ Test FAIL: Không trùng khớp${NC}"
fi
echo

# Demo với nhiều trường hợp test
echo -e "${BLUE}Demo 3: Test nhiều trường hợp${NC}"

test_cases=(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz" 
    "Test 123 !@#"
    "Caesar Cipher"
    "Ubuntu Linux"
)

for i in "${!test_cases[@]}"; do
    text="${test_cases[$i]}"
    echo "Test $((i+1)): $text"
    
    # Mã hóa
    echo -n "E:$text" > $DEVICE
    encoded=$(cat $DEVICE)
    
    # Giải mã
    echo -n "D:$encoded" > $DEVICE
    decoded=$(cat $DEVICE)
    
    if [[ "$text" == "$decoded" ]]; then
        echo -e "  ${GREEN}✓ PASS${NC}"
    else
        echo -e "  ${RED}✗ FAIL${NC}"
        echo "    Gốc: '$text'"
        echo "    Mã hóa: '$encoded'" 
        echo "    Giải mã: '$decoded'"
    fi
done

echo
echo -e "${BLUE}Demo hoàn thành!${NC}" 