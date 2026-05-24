const { createLogger, format, transports, Transport } = require('winston');
const Anthropic = require('@anthropic-ai/sdk');

const anthropic = new Anthropic({ apiKey: process.env.ANTHROPIC_API_KEY });

async function sendErrorToLLM(message) {
    try {
        const response = await anthropic.messages.create({
            model: 'claude-sonnet-4-6',
            max_tokens: 512,
            messages: [{
                role: 'user',
                content: `서버 에러 로그가 발생했습니다. 원인과 조치 방법을 간략히 분석해주세요.\n\n${message}`
            }]
        });
        return response.content[0].text;
    } catch {
        return null;
    }
}

class LLMErrorTransport extends Transport {
    log(info, callback) {
        if (info.level === 'error') {
            sendErrorToLLM(info.message).then(analysis => {
                if (analysis) {
                    fileLogger.info('[LLM 분석] ' + analysis);
                }
            });
        }
        callback();
    }
}

const fileLogger = createLogger({
    level: 'info',
    format: format.combine(
        format.timestamp(),
        format.printf(info => `${info.timestamp} ${info.level}: ${info.message}`)
    ),
    transports: [
        new transports.File({
            filename: `logs/node_${new Date().toISOString().split('T')[0]}.log`
        }),
        new transports.Console(),
        new LLMErrorTransport()
    ]
});

module.exports = fileLogger;