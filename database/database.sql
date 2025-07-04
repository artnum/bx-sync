
-- ** Contact related table **
CREATE TABLE IF NOT EXISTS contact_group (
    id BIGINT UNSIGNED PRIMARY KEY,
    name TEXT NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum),
    INDEX USING HASH (_deleted)
);

CREATE TABLE IF NOT EXISTS contact_sector (
    id BIGINT UNSIGNED PRIMARY KEY,
    name TEXT NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum),
    INDEX USING HASH (_deleted)
);

CREATE TABLE IF NOT EXISTS cs2c (
    contact_sector BIGINT UNSIGNED,
    contact BIGINT UNSIGNED,
    PRIMARY KEY(contact_sector, contact)
);

CREATE TABLE IF NOT EXISTS cg2c (
    contact_group BIGINT UNSIGNED,
    contact BIGINT UNSIGNED,
    PRIMARY KEY(contact_group, contact)
);

CREATE TABLE IF NOT EXISTS user (
    id BIGINT UNSIGNED PRIMARY KEY,
    salutation_type VARCHAR(20) NOT NULL,
    firstname VARCHAR(80) NOT NULL DEFAULT "",
    lastname VARCHAR(80) NOT NULL DEFAULT "",
    email TEXT NOT NULL,
    is_superadmin CHAR DEFAULT 0,
    is_accountant CHAR DEFAULT 0,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum),
    INDEX USING HASH (_deleted)
);

CREATE TABLE IF NOT EXISTS language (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(20) NOT NULL,
    decimal_point VARCHAR(5) NOT NULL DEFAULT '',
    thousands_separator VARCHAR(5) NOT NULL DEFAULT '',
    date_format_id BIGINT UNSIGNED NOT NULL DEFAULT 1,
    date_format VARCHAR(20) NOT NULL DEFAULT '',
    iso_639_1 VARCHAR(5) NOT NULL DEFAULT '',
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING BTREE(name),
    INDEX USING BTREE(iso_639_1),
    INDEX USING HASH (_deleted)
);

CREATE TABLE IF NOT EXISTS contact (
    id BIGINT UNSIGNED PRIMARY KEY,
    contact_type_id BIGINT UNSIGNED,
    salutation_id BIGINT UNSIGNED NULL,
    country CHAR(2) NULL,
    user_id BIGINT UNSIGNED,
    owner_id BIGINT UNSIGNED,
    title_id BIGINT UNSIGNED NULL,
    language_id BIGINT UNSIGNED NULL,
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
    _archived BOOLEAN DEFAULT FALSE,
    FOREIGN KEY (user_id) REFERENCES user(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    FOREIGN KEY (owner_id) REFERENCES user(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    FOREIGN KEY (language_id) REFERENCES language(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    INDEX USING HASH (_checksum)
);

-- ** Invoice related table **

CREATE TABLE IF NOT EXISTS  pr_project (
    id BIGINT UNSIGNED PRIMARY KEY,
    uuid BINARY(16) NOT NULL,
    nr VARCHAR(20) NOT NULL,
    name TEXT DEFAULT '',
    start_date VARCHAR(20) DEFAULT NULL,
    end_date VARCHAR(20) DEFAULT NULL,
    comment TEXT DEFAULT '',
    pr_state_id BIGINT UNSIGNED,
    pr_project_type_id BIGINT UNSIGNED,
    contact_id BIGINT UNSIGNED NOT NULL,
    contact_sub_id BIGINT UNSIGNED DEFAULT NULL,
    pr_invoice_type_id BIGINT UNSIGNED,
    pr_invoice_type_amount FLOAT,
    pr_budget_type_id BIGINT UNSIGNED,
    pr_budget_type_amount  FLOAT,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH(uuid),
    INDEX USING HASH(nr),
    INDEX USING HASH (_checksum),
    INDEX USING HASH (id),
    FOREIGN KEY (contact_id) REFERENCES contact(id) ON DELETE RESTRICT ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS account (
    id BIGINT UNSIGNED PRIMARY KEY,
    uuid BINARY(16) NOT NULL,

    INDEX USING HASH(uuid)
);

CREATE TABLE IF NOT EXISTS unit (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(30),
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);



CREATE TABLE IF NOT EXISTS invoice (
    id BIGINT UNSIGNED PRIMARY KEY,
    document_nr VARCHAR(255),
    title VARCHAR(80) DEFAULT NULL,
    contact_id BIGINT UNSIGNED,
    contact_sub_id BIGINT UNSIGNED,
    user_id BIGINT UNSIGNED,
    project_id BIGINT UNSIGNED,
    language_id BIGINT UNSIGNED,
    bank_account_id BIGINT UNSIGNED,
    currency_id BIGINT UNSIGNED,
    payment_type_id BIGINT UNSIGNED,
    header TEXT,
    footer TEXT,
    mwst_type INT,
    mwst_is_net TINYINT,
    show_position_taxes TINYINT,
    is_valid_from VARCHAR(10),
    is_valid_to VARCHAR(10),
    contact_address TEXT,
    kb_item_status_id INT,
    reference TEXT,
    api_reference TEXT,
    viewed_by_client_at VARCHAR(19),
    updated_at VARCHAR(19),
    esr_id INT,
    qr_invoice_id INT,
    template_slug VARCHAR(32),
    network_link TEXT DEFAULT '',
    total_gross FLOAT NOT NULL DEFAULT 0.0,
    total_net FLOAT NOT NULL DEFAULT 0.0,
    total_taxes FLOAT NOT NULL DEFAULT 0.0,
    total_received_payments FLOAT NOT NULL DEFAULT 0.0,
    total_credit_vouchers FLOAT NOT NULL DEFAULT 0.0,
    total_remaining_payments FLOAT NOT NULL DEFAULT 0.0,
    total FLOAT NOT NULL DEFAULT 0.0,
    total_rounding_difference FLOAT NOT NULL DEFAULT 0.0,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum),
    FOREIGN KEY (contact_id) REFERENCES contact(id) ON UPDATE CASCADE ON DELETE RESTRICT,
    FOREIGN KEY (project_id) REFERENCES pr_project(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS invoice_position (
    id BIGINT PRIMARY KEY,
    amount FLOAT NOT NULL,
    account_id BIGINT,
    unit_id BIGINT UNSIGNED DEFAULT NULL,
    tax_id BIGINT,
    tax_value FLOAT,
    description TEXT DEFAULT '',
    unit_price FLOAT,
    discount FLOAT DEFAULT 0.0,
    position INT,
    internal_position INT,
    type VARCHAR(20),
    parent_id BIGINT DEFAULT NULL,
    _invoice_id BIGINT UNSIGNED,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum),
    FOREIGN KEY (_invoice_id) REFERENCES invoice(id) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (unit_id) REFERENCES unit(id) ON UPDATE CASCADE ON DELETE RESTRICT

);

CREATE TABLE IF NOT EXISTS taxes (
  id BIGINT UNSIGNED PRIMARY KEY,
  uuid BINARY(16) NOT NULL,
  name VARCHAR(80) NOT NULL,
  code VARCHAR(80),
  digit INTEGER NOT NULL,
  type VARCHAR(80) NOT NULL,
  account_id BIGINT UNSIGNED NOT NULL,
  tax_settlement_type VARCHAR(80) NOT NULL,
  value FLOAT NOT NULL,
  net_tax_value VARCHAR(255) DEFAULT NULL,
  start_year INTEGER,
  end_year INTEGER,
  is_active BOOLEAN DEFAULT true,
  display_name VARCHAR(255),
  start_month INTEGER,
  end_month INTEGER,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
  INDEX USING HASH (_checksum)
);

