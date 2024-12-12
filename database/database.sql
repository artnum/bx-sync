CREATE TABLE IF NOT EXISTS contact_group (
    id BIGINT PRIMARY KEY,
    name TEXT NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
    INDEX USING HASH _checksum,
    INDEX USING HASH _deleted,
);

CREATE TABLE IF NOT EXISTS contact_sector (
    id BIGINT PRIMARY KEY,
    name TEXT NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
    INDEX USING HASH _checksum,
    INDEX USING HASH _deleted,
);

CREATE TABLE IF NOT EXISTS user (
    id BIGINT PRIMARY KEY,
    salutation_type VARCHAR(20) NOT NULL,
    firstname VARCHAR(80) NOT NULL DEFAULT "",
    lastname VARCHAR(80) NOT NULL DEFAULT "",
    email TEXT NOT NULL,
    is_superadmin CHAR DEFAULT 0,
    is_accountant CHAR DEFAULT 0,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH _checksum,
    INDEX USING HASH _deleted
);

CREATE TABLE IF NOT EXISTS contact (
    id BIGINT PRIMARY KEY,
    contact_type_id BIGINT,
    salutation_id BIGINT NULL,
    country CHAR(2) NULL,
    user_id BIGINT,
    owner_id BIGINT,
    title_id BIGINT NULL,
    salutation_form BIGINT NULL,
    postcode TEXT NULL,
    nr TEXT,
    name_1 TEXT NOT NULL,
    name_2 TEXT NULL,
    birthday TEXT NULL,
    address TEXT NULL,
    city TEXT NULL,
    mail TEXT NULL,
    mail_second TEXT NULL,
    phone_fixed TEXT NULL,
    phone_fixed_second TEXT NULL,
    phone_mobile TEXT NULL,
    fax TEXT NULL,
    url TEXT NULL,
    skype_name TEXT NULL,
    remarks TEXT NULL,
    updated_at TEXT NULL,
    profile_image TEXT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum),
    INDEX USING HASH (_deleted)
);

CREATE TABLE IF NOT EXISTS invoice_position (
    id BIGINT PRIMARY KEY,
    amount FLOAT NOT NULL,
    account_id BIGINT,
    unit_id BIGINT,
    tax_id BIGINT,
    tax_value FLOAT,
    description TEXT DEFAULT '',
    unit_price FLOAT,
    discount FLOAT DEFAULT 0.0,
    position INT,
    internal_position INT,
    type VARCHAR(20),
    parent_id BIGINT DEFAULT NULL,
    _invoice BIGINT,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH _checksum,
    INDEX USING HASH _deleted,
    FOREIGN KEY _invoice REFERENCES invoice(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS tax (
    id BIGINT PRIMARY KEY,
    percentage FLOAT,
    value FLOAT,
    _invoice BIGINT,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH _checksum,
    INDEX USING HASH _deleted,
    FOREIGN KEY _invoice REFERENCES invoice(id)
        ON UPDATE CASCADE
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS invoice (
    id BIGINT PRIMARY KEY,
    document_nr VARCHAR(255),
    title VARCHAR(80) DEFAULT NULL,
    contact_id BIGINT,
    contact_sub_id BIGINT,
    user_id BIGINT,
    project_id BIGINT,
    language_id BIGINT,
    bank_account_id BIGINT,
    currency_id BIGINT,
    payment_type_id BIGINT,
    header TEXT,
    footer TEXT,
    mwst_type INT,
    mwst_is_net TINYINT,
    show_position_taxes TINYINT,
    is_valid_from VARCHAR(10),
    is_valid_to VARCHAR(10),
    contact_address TEXT,
    kb_item_status INT,
    reference TEXT,
    api_reference TEXT,
    viewed_by_client_at VARCHAR(19),
    updated_at VARCHAR(19),
    esr_id INT,
    qr_invoice_id INT,
    template_slug VARCHAR(32),
    network_link TEXT DEFAULT '',
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH _checksum,
    INDEX USING HASH _deleted,
    FOREIGN KEY contact_id REFERENCES contact(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
)

