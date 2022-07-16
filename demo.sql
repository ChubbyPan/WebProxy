CREATE database space;
use space;
--- 表创建
CREATE TABLE IF NOT EXISTS `military_resource`(
    `id` INT UNSIGNED AUTO_INCREMENT,
   `military_resource_id` VARCHAR(40) NOT NULL COMMENT '旅:ax, 如a1 / 营:bx, 如b1',
   `name` VARCHAR(40),
   `resource_type` INT(10) NOT NULL COMMENT '旅:1，营:2',
   `superior` VARCHAR(10) NOT NULL COMMENT '上级id，旅上级为"0"，营上级为旅id',
   `coordinate` VARCHAR(20) NOT NULL COMMENT '当前坐标',
   `hit_rate` FLOAT NOT NUll COMMENT '打击概率',
   `value` FLOAT NOT NULL COMMENT '价值',
   `shape` VARCHAR(200) COMMENT 'item图标url',
   `status` INT(10) DEFAULT 1 COMMENT '生:1/死:0',
   `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
   `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY ( `id` ),
    unique key  ( `military_resource_id` )
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `combat_resource`(
    `id` INT UNSIGNED AUTO_INCREMENT,
   `combat_resource_id` VARCHAR(40) NOT NULL ,
   `name` VARCHAR(40) NOT NULL,
   `resource_type` INT(10) NOT NULL COMMENT '资源类型，导弹:1, 通讯:2, 传感器:3',
   `belong_to` VARCHAR(40) NOT NULL COMMENT '属于哪个军队资源', 
   `coordinate` VARCHAR(20) NOT NULL COMMENT '当前坐标',
   `hit_rate` FLOAT NOT NUll COMMENT '打击概率',
   `value` FLOAT NOT NULL COMMENT '价值',
   `shape` VARCHAR(200) NOT NULL COMMENT 'item图标url',
   `status` INT(10) NOT NULL DEFAULT 1 COMMENT '生:1/死:0',
   `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
   `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
   PRIMARY KEY ( `id` ),
   unique key  ( `combat_resource_id` ),
   FOREIGN KEY(belong_to) REFERENCES military_resource(military_resource_id)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `enemy_resource`(
    `id` INT UNSIGNED AUTO_INCREMENT,
   `enemy_resource_id` VARCHAR(40) NOT NULL,
   `name` VARCHAR(20) NOT NULL,
    `resource_type` INT(10) NOT NULL COMMENT '旅:1，营:2',
   `shape_type` INT(10) NOT NULL COMMENT '运行轨迹类型',
   `coordinate` VARCHAR(20) NOT NULL COMMENT '当前坐标',
   `hit_rate` FLOAT NOT NUll COMMENT '打击概率',
   `value` FLOAT NOT NULL COMMENT '价值',
   `status` INT(1) NOT NULL DEFAULT 1 COMMENT '生:1/死:0',
   `shape` VARCHAR(200) NOT NULL COMMENT 'item图标url',
   `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
   `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
      unique key  ( `enemy_resource_id` ),
   PRIMARY KEY ( `id` )
)ENGINE=InnoDB DEFAULT CHARSET=utf8;



--- 数据创建

--- 军队资源

INSERT INTO military_resource  (military_resource_id, name,resource_type, superior , coordinate, shape, value, hit_rate)  VALUES  ('a1', 'name', 1, '0', '200_200', 'url', 1.1);
INSERT INTO military_resource  (military_resource_id, name,resource_type, superior, coordinate, shape, value, hit_rate)  VALUES  ('a2', 'name', 1, '0', '200_300', 'url', 1.2);

INSERT INTO military_resource  (military_resource_id, name,resource_type, superior , coordinate, shape, value, hit_rate)  VALUES  ('b1', 'name', 2, 'a1',  '200_500', 'url',3.23);
INSERT INTO military_resource  (military_resource_id, name,resource_type, superior , coordinate, shape, value, hit_rate) VALUES  ('b2', 'name', 2, 'a1',  '200_500', 'url', 2.23);

INSERT INTO military_resource  (military_resource_id, name,resource_type, superior , coordinate, shape, value, hit_rate)  VALUES  ('b3', 'name', 2, 'a2',  '200_500', 'url', 2.43);
INSERT INTO military_resource  (military_resource_id, name,resource_type, superior , coordinate, shape, value, hit_rate)  VALUES  ('b4', 'name', 2, 'a2',  '200_500', 'url',4.4312);
INSERT INTO military_resource  (military_resource_id, name,resource_type, superior , coordinate, shape, value, hit_rate)  VALUES  ('b5', 'name', 2, 'a2',  '200_500', 'url',5.4532);

--- 【注】每次插两条记录，上级和下级都需定义一便

--- 作战资源

INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)  VALUES  ('m1', 'name', 1,  'b1', '210_500', 'url', 0.95, 0.85);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)  VALUES  ('m2', 'name', 1,  'b2', '220_500', 'url', 0.8, 0.75);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('c1', 'name', 2,  'b3', '230_500', 'url',1, 0.35);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('c2', 'name', 2,  'b4', '240_500', 'url',0.8, 0.7);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('s1', 'name', 3,  'b5', '250_500', 'url',0.9, 0.8);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('s2', 'name', 3,  'b6', '260_500', 'url',1, 0.45);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('a1', 'name', 4,  'b7', '270_500', 'url',0.9, 0.8);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('a2', 'name', 4,  'b8', '280_500', 'url',0.75, 0.7);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('d1', 'name', 5,  'b9', '290_500', 'url',1, 0.25);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('d2', 'name', 5,  'b10', '300_500', 'url',0.85, 0.75);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('f1', 'name', 6,  'b11', '310_500', 'url',0.65, 0.55);
INSERT INTO combat_resource  (combat_resource_id, name,resource_type, belong_to,  coordinate, shape, value, hit_rate)   VALUES  ('f2', 'name', 6,  'b12', '320_500', 'url',1, 0.5);




--- 敌方资源

INSERT INTO enemy_resource  (enemy_resource_id, name,shape_type,   coordinate, shape, value)  VALUES  ('r1', 'name', 1,  '260_500', 'url', 2.6534);
INSERT INTO enemy_resource  (enemy_resource_id, name,shape_type,   coordinate, shape, value)  VALUES  ('r2', 'name', 2,  '260_500', 'url', 2.4321);
INSERT INTO enemy_resource  (enemy_resource_id, name,shape_type,   coordinate, shape, value)  VALUES  ('r3', 'name', 3,  '260_500', 'url', 5.62354);


-- drop table combat_resource;
-- drop table military_resource;
-- drop table enemy_resource;

--- 检查插入
select * from military_resource;
select * from combat_resource;
select * from enemy_resource;
--- 数据获取
--- 找到敌方资源的运行轨道类型（圆、正方形...）
select shape_type from enemy_resource where enemy_resource_id = 'r3';

--- 获取b3营上级
select superior from military_resource where military_resource_id = 'b3';
--- 获取a1旅下级 
select military_resource_id from military_resource where superior = 'a1';

--- 获取b2营（军队资源）的导弹车id
SELECT combat_resource_id FROM combat_resource 
INNER JOIN military_resource ON combat_resource.belong_to = military_resource.military_resource_id
WHERE military_resource.military_resource_id = 'b2' and combat_resource.resource_type = 1;


--- 数据更新

--- 更新状态（死/0、活/1）

update status=0 from yyy where  enemy_resource_id = '1';
update status=1 from yyy where  enemy_resource_id = '1';

--- 更新坐标

update coordinate='1_1' from military_resource where  military_resource_id = '1';
update coordinate='2_1' from combat_resource where  combat_resource_id = '1';
update coordinate='3_1' from enemy_resource where  enemy_resource_id = '1';

