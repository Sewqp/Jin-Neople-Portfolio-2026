const express = require('express');
const router  = express.Router();
const pool    = require('../db');
const { logger } = require('../server');

// [거래소 목록 조회 — 판매 중 + 아이템 타입 필터 + 페이징]
router.get('/api/auction', async (req, res) => {
    const itemType = req.query.item_type || 0;
    const limit    = Number(req.query.limit)  || 20;
    const offset   = Number(req.query.offset) || 0;

    try {
        const [rows] = await pool.execute(`
            SELECT a.auction_id, id.item_name, a.price,
                   ii.enhance_level, a.expired_at
            FROM auction AS a
            JOIN item_instance AS ii ON a.item_instance_id = ii.item_instance_id
            JOIN item_dictionary AS id ON ii.item_dict_id = id.item_dict_id
            WHERE a.trade_status = 0 AND id.item_type = ?
            LIMIT ? OFFSET ?
        `, [itemType, limit, offset]);

        res.json(rows);
    } catch (error) {
        logger.error('거래소 목록 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

// [거래소 상세 조회]
router.get('/api/auction/:auctionId', async (req, res) => {
    const auctionId = req.params.auctionId;

    try {
        const [rows] = await pool.execute(`
            SELECT a.auction_id, id.item_name, a.price,
                   ii.enhance_level, a.expired_at
            FROM auction AS a
            JOIN item_instance AS ii ON a.item_instance_id = ii.item_instance_id
            JOIN item_dictionary AS id ON ii.item_dict_id = id.item_dict_id
            WHERE a.auction_id = ?
        `, [auctionId]);

        if (rows.length === 0) {
            return res.status(404).json({ message: '해당 경매를 찾을 수 없습니다.' });
        }

        res.json(rows[0]);
    } catch (error) {
        logger.error('거래소 상세 조회 오류: ' + error.message);
        res.status(500).json({ message: '서버 오류가 발생했습니다.' });
    }
});

module.exports = router;
