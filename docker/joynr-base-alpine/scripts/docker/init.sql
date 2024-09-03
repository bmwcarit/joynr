CREATE SEQUENCE public.hibernate_sequence
    INCREMENT 1
    START 1
    MINVALUE 1
    MAXVALUE 9223372036854775807
    CACHE 1;

CREATE TABLE IF NOT EXISTS public.customparameters
(
    id bigint NOT NULL,
    name character varying(255) COLLATE pg_catalog."default",
    value character varying(255) COLLATE pg_catalog."default",
    CONSTRAINT customparameters_pkey PRIMARY KEY (id)
);

CREATE SEQUENCE IF NOT EXISTS public.customparameters_SEQ increment by 1;

CREATE TABLE IF NOT EXISTS public.discovery_entries
(
    gbid character varying(255) COLLATE pg_catalog."default" NOT NULL,
    participantid character varying(255) COLLATE pg_catalog."default" NOT NULL,
    address oid,
    clustercontrollerid character varying(255) COLLATE pg_catalog."default",
    domain character varying(255) COLLATE pg_catalog."default",
    expirydatems bigint,
    interfacename character varying(255) COLLATE pg_catalog."default",
    lastseendatems bigint,
    majorversion integer,
    minorversion integer,
    priority bigint,
    scope integer,
    supportsonchangesubscriptions boolean,
    publickeyid character varying(255) COLLATE pg_catalog."default",
    CONSTRAINT discovery_entries_pkey PRIMARY KEY (gbid, participantid)
);

CREATE TABLE IF NOT EXISTS public.discovery_entries_customparameters
(
    globaldiscoveryentrypersisted_gbid character varying(255) COLLATE pg_catalog."default" NOT NULL,
    globaldiscoveryentrypersisted_participantid character varying(255) COLLATE pg_catalog."default" NOT NULL,
    customparameterlist_id bigint NOT NULL,
    CONSTRAINT customparameterlist_id_is_unique UNIQUE (customparameterlist_id),
    CONSTRAINT customparameterlist_id_references_customparameters FOREIGN KEY (customparameterlist_id)
        REFERENCES public.customparameters (id) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION,
    CONSTRAINT gbid_participantid_references_discovery_entries FOREIGN KEY (globaldiscoveryentrypersisted_gbid, globaldiscoveryentrypersisted_participantid)
        REFERENCES public.discovery_entries (gbid, participantid) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION
);

CREATE DATABASE "gcd-test"
    WITH
    OWNER = gcd
    ENCODING = 'UTF8'
    CONNECTION LIMIT = -1;

