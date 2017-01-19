-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Ruby on Rails LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S
local table = _G.table

local M = {_NAME = 'rails'}

-- Whitespace
local ws = token(l.WHITESPACE, l.space^1)

-- Functions.

local actionpack = token(l.FUNCTION, word_match{
  'before_filter', 'skip_before_filter', 'skip_after_filter', 'after_filter',
  'around_filter', 'filter', 'filter_parameter_logging', 'layout',
  'require_dependency', 'render', 'render_action', 'render_text', 'render_file',
  'render_template', 'render_nothing', 'render_component',
  'render_without_layout', 'rescue_from', 'url_for', 'redirect_to',
  'redirect_to_path', 'redirect_to_url', 'respond_to', 'helper',
  'helper_method', 'model', 'service', 'observer', 'serialize', 'scaffold',
  'verify', 'hide_action'
})

local view_helpers = token(l.FUNCTION, word_match{
  'check_box', 'content_for', 'error_messages_for', 'form_for', 'fields_for',
  'file_field', 'hidden_field', 'image_submit_tag', 'label', 'link_to',
  'password_field', 'radio_button', 'submit', 'text_field', 'text_area'
})

local activerecord = token(l.FUNCTION, word_match{
  'after_create', 'after_destroy', 'after_save', 'after_update',
  'after_validation', 'after_validation_on_create',
  'after_validation_on_update', 'before_create', 'before_destroy',
  'before_save', 'before_update', 'before_validation',
  'before_validation_on_create', 'before_validation_on_update', 'composed_of',
  'belongs_to', 'has_one', 'has_many', 'has_and_belongs_to_many', 'validate',
  'validates', 'validate_on_create', 'validates_numericality_of',
  'validate_on_update', 'validates_acceptance_of', 'validates_associated',
  'validates_confirmation_of', 'validates_each', 'validates_format_of',
  'validates_inclusion_of', 'validates_exclusion_of', 'validates_length_of',
  'validates_presence_of', 'validates_size_of', 'validates_uniqueness_of',
  'attr_protected', 'attr_accessible', 'attr_readonly',
  'accepts_nested_attributes_for', 'default_scope', 'scope'
})

local active_support = token(l.FUNCTION, word_match{
  'alias_method_chain', 'alias_attribute', 'delegate', 'cattr_accessor',
  'mattr_accessor', 'returning', 'memoize'
})

-- Extend Ruby lexer to include Rails methods.
local ruby = l.load('ruby')
local _rules = ruby._rules
_rules[1] = {'whitespace', ws}
table.insert(_rules, 3, {'actionpack', actionpack})
table.insert(_rules, 4, {'view_helpers', view_helpers})
table.insert(_rules, 5, {'activerecord', activerecord})
table.insert(_rules, 6, {'active_support', active_support})
M._rules = _rules
M._foldsymbols = ruby._foldsymbols

return M
