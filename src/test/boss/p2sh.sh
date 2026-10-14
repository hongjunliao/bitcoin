#!/bin/bash
# 使用 ${bcli} 创建 2/3 P2SH 多签地址 的方法

create_2of3_p2sh() {
   local bcli="$1"
  local dirsav="$2"
    # 检查依赖
    if ! command -v ${bcli} &> /dev/null; then
        echo "错误: 未找到 ${bcli}" >&2
        return 1
    fi
    if ! command -v jq &> /dev/null; then
        echo "错误: 未找到 jq，请先安装 jq" >&2
        return 1
    fi

    # 生成带标签的 3 个地址（方便后续识别）
    local label="multisig_2of3_$(date +%s)"
    local addr1 addr2 addr3
    addr1=$(${bcli} getnewaddress "${label}_key1" legacy)
    addr2=$(${bcli} getnewaddress "${label}_key2" legacy)
    addr3=$(${bcli} getnewaddress "${label}_key3" legacy)

    # 提取公钥
    local pubkey1 pubkey2 pubkey3
    pubkey1=$(${bcli} getaddressinfo "$addr1" | jq -r '.pubkey')
    pubkey2=$(${bcli} getaddressinfo "$addr2" | jq -r '.pubkey')
    pubkey3=$(${bcli} getaddressinfo "$addr3" | jq -r '.pubkey')

    if [[ -z "$pubkey1" || -z "$pubkey2" || -z "$pubkey3" || "$pubkey1" == "null" ]]; then
        echo "错误: 无法获取公钥" >&2
        return 1
    fi

    # 创建 2-of-3 P2SH 多签
    local result
    result=$($bcli createmultisig 2 "[\"$pubkey1\",\"$pubkey2\",\"$pubkey3\"]")

    local multisig_address redeem_script descriptor
    multisig_address=$(echo "$result" | jq -r '.address')
    redeem_script=$(echo "$result" | jq -r '.redeemScript')
    descriptor=$(echo "$result" | jq -r '.descriptor')

    if [[ -z "$multisig_address" || "$multisig_address" == "null" ]]; then
        echo "错误: 创建多签地址失败" >&2
        return 1
    fi

    # 使用 importdescriptors 导入描述符（现代推荐方式）
    # 因为私钥已经在钱包里（通过 getnewaddress 生成），所以导入公钥描述符即可签名
    local import_result
    import_result=$($bcli importdescriptors "[{
        \"desc\": \"$descriptor\",
        \"timestamp\": \"now\",
        \"active\": false,
        \"internal\": false
    }]")

    # 检查导入是否成功
    local import_success
    import_success=$(echo "$import_result" | jq -r '.[0].success')
    if [[ "$import_success" != "true" ]]; then
        echo "警告: 描述符导入可能失败" >&2
        echo "$import_result" | jq . >&2
    fi

    # 构建完整信息并保存
    local info
    info=$(jq -n \
        --arg address "$multisig_address" \
        --arg redeemScript "$redeem_script" \
        --arg descriptor "$descriptor" \
        --arg addr1 "$addr1" \
        --arg addr2 "$addr2" \
        --arg addr3 "$addr3" \
        --arg pubkey1 "$pubkey1" \
        --arg pubkey2 "$pubkey2" \
        --arg pubkey3 "$pubkey3" \
        --arg label "$label" \
        '{
            address: $address,
            redeemScript: $redeemScript,
            descriptor: $descriptor,
            required: 2,
            total: 3,
            label: $label,
            keys: [
                {address: $addr1, pubkey: $pubkey1},
                {address: $addr2, pubkey: $pubkey2},
                {address: $addr3, pubkey: $pubkey3}
            ]
        }')


    # 保存到文件（文件名带时间戳，防止覆盖）
    local filename="$dirsav/multisig_2of3_${label}.json"
    echo "$info" > "$filename"
    echo "完整多签信息已保存到: $filename" >&2
    echo "$info" | jq . >&2
    echo "----------------------------------------" >&2
    echo "重要提示: 请务必备份这个 JSON 文件和钱包！" >&2
    echo "后续花费时需要用到 redeemScript 和至少 2 个私钥。" >&2

    # 返回多签地址
    echo "$multisig_address"
}

create_2of3_p2sh "$1" "$2"