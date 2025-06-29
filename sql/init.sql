-- 创建数据库
CREATE DATABASE IF NOT EXISTS smcell_db;

-- 使用数据库
USE smcell_db;

-- 创建组网关系表
CREATE TABLE IF NOT EXISTS vsatDetails (
    id INT AUTO_INCREMENT PRIMARY KEY,
    vsatIP VARCHAR(15) NOT NULL DEFAULT '',
    toLabel VARCHAR(15) NOT NULL DEFAULT '',
    UNIQUE KEY (vsatIP)
);

-- 插入示例数据
INSERT INTO vsatDetails (vsatIP, toLabel)
VALUES 
('10.0.0.1', '1'),
('10.0.0.2', '2'),
('10.0.0.3', '3');