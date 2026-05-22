#!/usr/bin/env bash
echo "🛡️ Pre-Flash Checklist for CrossInk-BookerLam"
echo "----------------------------------------------"
echo "[ ] 1. Git diff đã review?"
echo "[ ] 2. pio run -e ai-check pass?"
echo "[ ] 3. pio run -e tiny build thành công?"
echo "[ ] 4. SPI config X3 ≤10MHz, X4 ≤40MHz?"
echo "[ ] 5. Framebuffer vẫn static?"
echo "[ ] 6. Đã backup .crosspoint/ cache (nếu cần)?"
read -p "Tiếp tục flash? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    pio run -e tiny --target upload
else
    echo "🛑 Hủy flash."
fi
