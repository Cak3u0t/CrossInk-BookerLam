#!/usr/bin/env bash
set -e
echo "🔍 [AI-Check] CrossInk-BookerLam Validation"
echo "================================================"

echo "🔍 [1/4] Checking hardware anti-patterns..."
# SPI clock checks (X3 vs X4)
if grep -rE "clock_speed_hz.*>[[:space:]]*10000000" src/drivers/DisplayX3.cpp 2>/dev/null; then
    echo "❌ FAIL: X3 SPI clock > 10 MHz!" && exit 1
fi
if grep -rE "malloc.*framebuffer|free.*display_buffer" src/ 2>/dev/null; then
    echo "❌ FAIL: Dynamic alloc for framebuffer!" && exit 1
fi

# BUSY pin timeout check
BUSY_WARNINGS=$(grep -rn "while.*BUSY" src/ 2>/dev/null | grep -v "timeout\|xTaskNotifyWait" || true)
if [ -n "$BUSY_WARNINGS" ]; then
    echo "⚠️  WARN: BUSY wait without timeout:" && echo "$BUSY_WARNINGS"
fi
echo "✅ [1/4] Hardware constraints passed."

echo "🔍 [2/4] Running clang-tidy..."
if command -v clang-tidy &> /dev/null; then
    # Tìm compile_commands.json từ PlatformIO
    COMP_DB=$(find .pio/build/ -name compile_commands.json -type f 2>/dev/null | head -1)
    if [ -n "$COMP_DB" ]; then
        find src/ -type f \( -name "*.c" -o -name "*.cpp" \) | \
        xargs clang-tidy -p "$(dirname "$COMP_DB")" -- -Wno-unknown-pragmas 2>/dev/null || \
        echo "⚠️  clang-tidy warnings found (review manually)."
    else
        echo "⚠️  compile_commands.json not found. Run 'pio run' first to generate."
    fi
else
    echo "⚠️  clang-tidy not installed."
fi

echo "🔍 [3/4] Running cppcheck..."
if command -v cppcheck &> /dev/null; then
    cppcheck --enable=warning,style --suppress=missingIncludeSystem \
             --suppress=unmatchedSuppression src/ 2>/dev/null || \
    echo "⚠️  cppcheck warnings found (review manually)."
else
    echo "⚠️  cppcheck not installed."
fi

echo "================================================"
echo "✅ AI Safety Checks Complete."
echo "📋 Next: Review warnings → pio run → Manual flash"
