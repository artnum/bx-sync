
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
    account_no VARCHAR(20) DEFAULT NULL,
    name TEXT,
    account_type INT DEFAULT NULL,
    tax_id BIGINT UNSIGNED DEFAULT NULL,
    fibu_account_group_id BIGINT UNSIGNED DEFAULT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    is_locked BOOLEAN DEFAULT FALSE,
    _checksum BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH(uuid),
    INDEX USING HASH (_checksum)
);

ALTER TABLE account ADD COLUMN IF NOT EXISTS account_no VARCHAR(20) DEFAULT NULL;
ALTER TABLE account ADD COLUMN IF NOT EXISTS name TEXT;
ALTER TABLE account ADD COLUMN IF NOT EXISTS account_type INT DEFAULT NULL;
ALTER TABLE account ADD COLUMN IF NOT EXISTS tax_id BIGINT UNSIGNED DEFAULT NULL;
ALTER TABLE account ADD COLUMN IF NOT EXISTS fibu_account_group_id BIGINT UNSIGNED DEFAULT NULL;
ALTER TABLE account ADD COLUMN IF NOT EXISTS is_active BOOLEAN DEFAULT TRUE;
ALTER TABLE account ADD COLUMN IF NOT EXISTS is_locked BOOLEAN DEFAULT FALSE;
ALTER TABLE account ADD COLUMN IF NOT EXISTS _checksum BIGINT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE account ADD COLUMN IF NOT EXISTS _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0;
ALTER TABLE account ADD COLUMN IF NOT EXISTS _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0;

CREATE TABLE IF NOT EXISTS unit (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(30),
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS salutation (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum)
);

CREATE TABLE IF NOT EXISTS title (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum)
);

CREATE TABLE IF NOT EXISTS payment_type (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum)
);

CREATE TABLE IF NOT EXISTS currency (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(16) NOT NULL,
    round_factor DOUBLE DEFAULT NULL,
    exchange_rate DOUBLE DEFAULT NULL,
    exchange_rate_id BIGINT UNSIGNED DEFAULT NULL,
    ratio DOUBLE DEFAULT NULL,
    source VARCHAR(80) DEFAULT NULL,
    source_reason VARCHAR(80) DEFAULT NULL,
    exchange_rate_date VARCHAR(20) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    INDEX USING HASH (_checksum)
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
    id BIGINT NOT NULL,
    amount FLOAT DEFAULT NULL,
    account_id BIGINT DEFAULT NULL,
    unit_id BIGINT UNSIGNED DEFAULT NULL,
    tax_id BIGINT DEFAULT NULL,
    tax_value FLOAT DEFAULT NULL,
    description TEXT DEFAULT '',
    unit_price FLOAT DEFAULT NULL,
    discount FLOAT DEFAULT NULL,
    position INT DEFAULT NULL,
    internal_position INT DEFAULT NULL,
    type VARCHAR(32) DEFAULT NULL,
    parent_id BIGINT DEFAULT NULL,
    _invoice_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _invoice_id),
    INDEX USING HASH (_checksum),
    FOREIGN KEY (_invoice_id) REFERENCES invoice(id) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY (unit_id) REFERENCES unit(id) ON UPDATE CASCADE ON DELETE RESTRICT
);

ALTER TABLE invoice_position MODIFY amount FLOAT DEFAULT NULL;
ALTER TABLE invoice_position MODIFY type VARCHAR(32) DEFAULT NULL;

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

-- ** Step 2: remaining GET resources (no payroll, no file blobs) **

CREATE TABLE IF NOT EXISTS country (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    name_short VARCHAR(16) DEFAULT NULL,
    iso3166_alpha2 CHAR(2) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS account_group (
    id BIGINT UNSIGNED PRIMARY KEY,
    uuid BINARY(16) DEFAULT NULL,
    account_no VARCHAR(20) DEFAULT NULL,
    name TEXT,
    parent_fibu_account_group_id BIGINT UNSIGNED DEFAULT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    is_locked BOOLEAN DEFAULT FALSE,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS client_service (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    default_is_billable BOOLEAN DEFAULT FALSE,
    default_price_per_hour DOUBLE DEFAULT NULL,
    account_id BIGINT UNSIGNED DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS communication_kind (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS pr_project_state (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS pr_project_type (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS timesheet_status (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS todo_status (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS todo_priority (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS stock (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS stock_place (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(80) NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS bank_account (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(120) DEFAULT NULL,
    owner TEXT,
    owner_address TEXT,
    owner_zip VARCHAR(20) DEFAULT NULL,
    owner_city VARCHAR(80) DEFAULT NULL,
    owner_country_code CHAR(2) DEFAULT NULL,
    bc_nr VARCHAR(32) DEFAULT NULL,
    bank_name VARCHAR(120) DEFAULT NULL,
    bank_nr VARCHAR(64) DEFAULT NULL,
    bank_account_nr VARCHAR(64) DEFAULT NULL,
    iban_nr VARCHAR(64) DEFAULT NULL,
    currency_id BIGINT UNSIGNED DEFAULT NULL,
    account_id BIGINT UNSIGNED DEFAULT NULL,
    remarks TEXT,
    invoice_mode VARCHAR(32) DEFAULT NULL,
    qr_invoice_iban VARCHAR(64) DEFAULT NULL,
    type VARCHAR(32) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS company_profile (
    id BIGINT UNSIGNED PRIMARY KEY,
    name VARCHAR(160) DEFAULT NULL,
    address TEXT,
    address_nr VARCHAR(20) DEFAULT NULL,
    postcode VARCHAR(20) DEFAULT NULL,
    city VARCHAR(80) DEFAULT NULL,
    country_id BIGINT UNSIGNED DEFAULT NULL,
    legal_form VARCHAR(80) DEFAULT NULL,
    country_name VARCHAR(80) DEFAULT NULL,
    mail TEXT,
    phone_fixed VARCHAR(80) DEFAULT NULL,
    phone_mobile VARCHAR(80) DEFAULT NULL,
    fax VARCHAR(80) DEFAULT NULL,
    url TEXT,
    ust_id_nr VARCHAR(80) DEFAULT NULL,
    mwst_nr VARCHAR(80) DEFAULT NULL,
    trade_register_nr VARCHAR(80) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS calendar_year (
    id BIGINT UNSIGNED PRIMARY KEY,
    start VARCHAR(20) DEFAULT NULL,
    end VARCHAR(20) DEFAULT NULL,
    is_vat_subject BOOLEAN DEFAULT NULL,
    is_annual_reporting BOOLEAN DEFAULT NULL,
    created_at VARCHAR(40) DEFAULT NULL,
    updated_at VARCHAR(40) DEFAULT NULL,
    vat_accounting_method VARCHAR(40) DEFAULT NULL,
    vat_accounting_type VARCHAR(40) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS business_year (
    id BIGINT UNSIGNED PRIMARY KEY,
    start VARCHAR(20) DEFAULT NULL,
    end VARCHAR(20) DEFAULT NULL,
    status VARCHAR(40) DEFAULT NULL,
    closed_at VARCHAR(20) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS vat_period (
    id BIGINT UNSIGNED PRIMARY KEY,
    start VARCHAR(20) DEFAULT NULL,
    end VARCHAR(20) DEFAULT NULL,
    type VARCHAR(40) DEFAULT NULL,
    status VARCHAR(40) DEFAULT NULL,
    closed_at VARCHAR(20) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS contact_relation (
    id BIGINT UNSIGNED PRIMARY KEY,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    contact_sub_id BIGINT UNSIGNED DEFAULT NULL,
    description TEXT,
    updated_at VARCHAR(32) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS additional_address (
    id BIGINT UNSIGNED NOT NULL,
    name TEXT,
    name_addition TEXT,
    address TEXT,
    street_name TEXT,
    house_number VARCHAR(20) DEFAULT NULL,
    address_addition TEXT,
    postcode VARCHAR(20) DEFAULT NULL,
    city VARCHAR(80) DEFAULT NULL,
    country_id BIGINT UNSIGNED DEFAULT NULL,
    subject TEXT,
    description TEXT,
    _contact_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _contact_id),
    FOREIGN KEY (_contact_id) REFERENCES contact(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS article (
    id BIGINT UNSIGNED PRIMARY KEY,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    article_type_id BIGINT UNSIGNED DEFAULT NULL,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    intern_code VARCHAR(80) DEFAULT NULL,
    intern_name TEXT,
    intern_description TEXT,
    purchase_price DOUBLE DEFAULT NULL,
    sale_price DOUBLE DEFAULT NULL,
    currency_id BIGINT UNSIGNED DEFAULT NULL,
    tax_id BIGINT UNSIGNED DEFAULT NULL,
    unit_id BIGINT UNSIGNED DEFAULT NULL,
    is_stock BOOLEAN DEFAULT FALSE,
    stock_id BIGINT UNSIGNED DEFAULT NULL,
    stock_place_id BIGINT UNSIGNED DEFAULT NULL,
    stock_nr DOUBLE DEFAULT NULL,
    remarks TEXT,
    article_group_id BIGINT UNSIGNED DEFAULT NULL,
    account_id BIGINT UNSIGNED DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS note (
    id BIGINT UNSIGNED PRIMARY KEY,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    event_start VARCHAR(32) DEFAULT NULL,
    subject TEXT,
    info TEXT,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    project_id BIGINT UNSIGNED DEFAULT NULL,
    entry_id BIGINT UNSIGNED DEFAULT NULL,
    module_id BIGINT UNSIGNED DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS task (
    id BIGINT UNSIGNED PRIMARY KEY,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    finish_date VARCHAR(40) DEFAULT NULL,
    subject TEXT,
    info TEXT,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    sub_contact_id BIGINT UNSIGNED DEFAULT NULL,
    project_id BIGINT UNSIGNED DEFAULT NULL,
    todo_status_id BIGINT UNSIGNED DEFAULT NULL,
    todo_priority_id BIGINT UNSIGNED DEFAULT NULL,
    has_reminder BOOLEAN DEFAULT FALSE,
    communication_kind_id BIGINT UNSIGNED DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS timesheet (
    id BIGINT UNSIGNED PRIMARY KEY,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    status_id BIGINT UNSIGNED DEFAULT NULL,
    client_service_id BIGINT UNSIGNED DEFAULT NULL,
    text TEXT,
    allowable_bill BOOLEAN DEFAULT NULL,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    sub_contact_id BIGINT UNSIGNED DEFAULT NULL,
    pr_project_id BIGINT UNSIGNED DEFAULT NULL,
    pr_package_id BIGINT UNSIGNED DEFAULT NULL,
    pr_milestone_id BIGINT UNSIGNED DEFAULT NULL,
    estimated_time VARCHAR(16) DEFAULT NULL,
    date VARCHAR(20) DEFAULT NULL,
    duration VARCHAR(16) DEFAULT NULL,
    running BOOLEAN DEFAULT FALSE,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS kb_quote (
    id BIGINT UNSIGNED PRIMARY KEY,
    document_nr VARCHAR(255) DEFAULT NULL,
    title VARCHAR(160) DEFAULT NULL,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    contact_sub_id BIGINT UNSIGNED DEFAULT NULL,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    project_id BIGINT UNSIGNED DEFAULT NULL,
    language_id BIGINT UNSIGNED DEFAULT NULL,
    bank_account_id BIGINT UNSIGNED DEFAULT NULL,
    currency_id BIGINT UNSIGNED DEFAULT NULL,
    payment_type_id BIGINT UNSIGNED DEFAULT NULL,
    header TEXT,
    footer TEXT,
    total_gross DOUBLE DEFAULT NULL,
    total_net DOUBLE DEFAULT NULL,
    total_taxes DOUBLE DEFAULT NULL,
    total DOUBLE DEFAULT NULL,
    total_rounding_difference DOUBLE DEFAULT NULL,
    mwst_type INT DEFAULT NULL,
    mwst_is_net TINYINT DEFAULT NULL,
    show_position_taxes TINYINT DEFAULT NULL,
    is_valid_from VARCHAR(20) DEFAULT NULL,
    is_valid_until VARCHAR(20) DEFAULT NULL,
    contact_address TEXT,
    kb_item_status_id INT DEFAULT NULL,
    updated_at VARCHAR(32) DEFAULT NULL,
    template_slug VARCHAR(64) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS kb_order (
    id BIGINT UNSIGNED PRIMARY KEY,
    document_nr VARCHAR(255) DEFAULT NULL,
    title VARCHAR(160) DEFAULT NULL,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    contact_sub_id BIGINT UNSIGNED DEFAULT NULL,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    project_id BIGINT UNSIGNED DEFAULT NULL,
    language_id BIGINT UNSIGNED DEFAULT NULL,
    bank_account_id BIGINT UNSIGNED DEFAULT NULL,
    currency_id BIGINT UNSIGNED DEFAULT NULL,
    payment_type_id BIGINT UNSIGNED DEFAULT NULL,
    header TEXT,
    footer TEXT,
    total_gross DOUBLE DEFAULT NULL,
    total_net DOUBLE DEFAULT NULL,
    total_taxes DOUBLE DEFAULT NULL,
    total DOUBLE DEFAULT NULL,
    total_rounding_difference DOUBLE DEFAULT NULL,
    mwst_type INT DEFAULT NULL,
    mwst_is_net TINYINT DEFAULT NULL,
    is_valid_from VARCHAR(20) DEFAULT NULL,
    contact_address TEXT,
    kb_item_status_id INT DEFAULT NULL,
    updated_at VARCHAR(32) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS kb_delivery (
    id BIGINT UNSIGNED PRIMARY KEY,
    document_nr VARCHAR(255) DEFAULT NULL,
    title VARCHAR(160) DEFAULT NULL,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    project_id BIGINT UNSIGNED DEFAULT NULL,
    language_id BIGINT UNSIGNED DEFAULT NULL,
    kb_item_status_id INT DEFAULT NULL,
    is_valid_from VARCHAR(20) DEFAULT NULL,
    contact_address TEXT,
    updated_at VARCHAR(32) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS quote_position (
    id BIGINT NOT NULL,
    amount FLOAT DEFAULT NULL,
    account_id BIGINT DEFAULT NULL,
    unit_id BIGINT UNSIGNED DEFAULT NULL,
    tax_id BIGINT DEFAULT NULL,
    tax_value FLOAT DEFAULT NULL,
    description TEXT,
    unit_price FLOAT DEFAULT NULL,
    discount FLOAT DEFAULT NULL,
    position INT DEFAULT NULL,
    internal_position INT DEFAULT NULL,
    type VARCHAR(32) DEFAULT NULL,
    parent_id BIGINT DEFAULT NULL,
    _quote_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _quote_id)
);

CREATE TABLE IF NOT EXISTS order_position (
    id BIGINT NOT NULL,
    amount FLOAT DEFAULT NULL,
    account_id BIGINT DEFAULT NULL,
    unit_id BIGINT UNSIGNED DEFAULT NULL,
    tax_id BIGINT DEFAULT NULL,
    tax_value FLOAT DEFAULT NULL,
    description TEXT,
    unit_price FLOAT DEFAULT NULL,
    discount FLOAT DEFAULT NULL,
    position INT DEFAULT NULL,
    internal_position INT DEFAULT NULL,
    type VARCHAR(32) DEFAULT NULL,
    parent_id BIGINT DEFAULT NULL,
    _order_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _order_id)
);

CREATE TABLE IF NOT EXISTS invoice_payment (
    id BIGINT UNSIGNED NOT NULL,
    date VARCHAR(20) DEFAULT NULL,
    value DOUBLE DEFAULT NULL,
    bank_account_id BIGINT UNSIGNED DEFAULT NULL,
    title TEXT,
    is_cash_discount BOOLEAN DEFAULT NULL,
    kb_invoice_id BIGINT UNSIGNED DEFAULT NULL,
    _invoice_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _invoice_id),
    FOREIGN KEY (_invoice_id) REFERENCES invoice(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS invoice_reminder (
    id BIGINT UNSIGNED NOT NULL,
    kb_invoice_id BIGINT UNSIGNED DEFAULT NULL,
    title TEXT,
    is_valid_from VARCHAR(20) DEFAULT NULL,
    is_valid_to VARCHAR(20) DEFAULT NULL,
    reminder_level INT DEFAULT NULL,
    is_sent BOOLEAN DEFAULT NULL,
    remaining_price DOUBLE DEFAULT NULL,
    _invoice_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _invoice_id),
    FOREIGN KEY (_invoice_id) REFERENCES invoice(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS kb_comment (
    id BIGINT UNSIGNED NOT NULL,
    text TEXT,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    user_name VARCHAR(160) DEFAULT NULL,
    date VARCHAR(32) DEFAULT NULL,
    is_public BOOLEAN DEFAULT NULL,
    _document_type VARCHAR(20) NOT NULL,
    _document_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _document_type, _document_id)
);

CREATE TABLE IF NOT EXISTS project_milestone (
    id BIGINT UNSIGNED NOT NULL,
    name TEXT,
    end_date VARCHAR(20) DEFAULT NULL,
    comment TEXT,
    pr_parent_milestone_id BIGINT UNSIGNED DEFAULT NULL,
    _project_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _project_id),
    FOREIGN KEY (_project_id) REFERENCES pr_project(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS project_package (
    id BIGINT UNSIGNED NOT NULL,
    name TEXT,
    spent_time_in_hours DOUBLE DEFAULT NULL,
    estimated_time_in_hours DOUBLE DEFAULT NULL,
    comment TEXT,
    pr_milestone_id BIGINT UNSIGNED DEFAULT NULL,
    _project_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _project_id),
    FOREIGN KEY (_project_id) REFERENCES pr_project(id) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS purchase_order (
    id BIGINT UNSIGNED PRIMARY KEY,
    document_nr VARCHAR(255) DEFAULT NULL,
    title TEXT,
    contact_id BIGINT UNSIGNED DEFAULT NULL,
    contact_sub_id BIGINT UNSIGNED DEFAULT NULL,
    user_id BIGINT UNSIGNED DEFAULT NULL,
    project_id BIGINT UNSIGNED DEFAULT NULL,
    language_id BIGINT UNSIGNED DEFAULT NULL,
    bank_account_id BIGINT UNSIGNED DEFAULT NULL,
    currency_id BIGINT UNSIGNED DEFAULT NULL,
    payment_type_id BIGINT UNSIGNED DEFAULT NULL,
    header TEXT,
    footer TEXT,
    template_slug VARCHAR(64) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS purchase_bill (
    id CHAR(36) PRIMARY KEY,
    created_at VARCHAR(40) DEFAULT NULL,
    document_no VARCHAR(80) DEFAULT NULL,
    status VARCHAR(40) DEFAULT NULL,
    vendor_ref TEXT,
    vendor TEXT,
    title TEXT,
    currency_code VARCHAR(8) DEFAULT NULL,
    pending_amount DOUBLE DEFAULT NULL,
    net DOUBLE DEFAULT NULL,
    gross DOUBLE DEFAULT NULL,
    bill_date VARCHAR(20) DEFAULT NULL,
    due_date VARCHAR(20) DEFAULT NULL,
    overdue BOOLEAN DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS expense (
    id CHAR(36) PRIMARY KEY,
    created_at VARCHAR(40) DEFAULT NULL,
    document_no VARCHAR(80) DEFAULT NULL,
    status VARCHAR(40) DEFAULT NULL,
    vendor TEXT,
    title TEXT,
    currency_code VARCHAR(8) DEFAULT NULL,
    paid_on VARCHAR(20) DEFAULT NULL,
    booking_account_id BIGINT UNSIGNED DEFAULT NULL,
    net DOUBLE DEFAULT NULL,
    gross DOUBLE DEFAULT NULL,
    chargeable_contact_id BIGINT UNSIGNED DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS manual_entry (
    id BIGINT UNSIGNED PRIMARY KEY,
    type VARCHAR(40) DEFAULT NULL,
    date VARCHAR(20) DEFAULT NULL,
    reference_nr TEXT,
    created_by_user_id BIGINT UNSIGNED DEFAULT NULL,
    edited_by_user_id BIGINT UNSIGNED DEFAULT NULL,
    is_locked BOOLEAN DEFAULT NULL,
    locked_info VARCHAR(80) DEFAULT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS manual_entry_line (
    id BIGINT UNSIGNED NOT NULL,
    date VARCHAR(20) DEFAULT NULL,
    debit_account_id BIGINT UNSIGNED DEFAULT NULL,
    credit_account_id BIGINT UNSIGNED DEFAULT NULL,
    tax_id BIGINT UNSIGNED DEFAULT NULL,
    description TEXT,
    amount DOUBLE DEFAULT NULL,
    currency_id BIGINT UNSIGNED DEFAULT NULL,
    _entry_id BIGINT UNSIGNED NOT NULL,
    _checksum BIGINT UNSIGNED NOT NULL,
    _last_updated BIGINT UNSIGNED NOT NULL DEFAULT 0,
    _deleted BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id, _entry_id)
);
