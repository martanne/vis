-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Ruby on Rails LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rails', {inherit = lexer.load('ruby')})

-- Whitespace
lex:modify_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Functions.
lex:modify_rule('function', token(lexer.FUNCTION, word_match{
  -- ActionPack.
  'before_filter', 'skip_before_filter', 'skip_after_filter', 'after_filter', 'around_filter',
  'filter', 'filter_parameter_logging', 'layout', 'require_dependency', 'render', 'render_action',
  'render_text', 'render_file', 'render_template', 'render_nothing', 'render_component',
  'render_without_layout', 'rescue_from', 'url_for', 'redirect_to', 'redirect_to_path',
  'redirect_to_url', 'respond_to', 'helper', 'helper_method', 'model', 'service', 'observer',
  'serialize', 'scaffold', 'verify', 'hide_action',
  -- View helpers.
  'check_box', 'content_for', 'error_messages_for', 'form_for', 'fields_for', 'file_field',
  'hidden_field', 'image_submit_tag', 'label', 'link_to', 'password_field', 'radio_button',
  'submit', 'text_field', 'text_area',
  -- ActiveRecord.
  'after_create', 'after_destroy', 'after_save', 'after_update', 'after_validation',
  'after_validation_on_create', 'after_validation_on_update', 'before_create', 'before_destroy',
  'before_save', 'before_update', 'before_validation', 'before_validation_on_create',
  'before_validation_on_update', 'composed_of', 'belongs_to', 'has_one', 'has_many',
  'has_and_belongs_to_many', 'validate', 'validates', 'validate_on_create',
  'validates_numericality_of', 'validate_on_update', 'validates_acceptance_of',
  'validates_associated', 'validates_confirmation_of', 'validates_each', 'validates_format_of',
  'validates_inclusion_of', 'validates_exclusion_of', 'validates_length_of',
  'validates_presence_of', 'validates_size_of', 'validates_uniqueness_of', --
  'attr_protected', 'attr_accessible', 'attr_readonly', 'accepts_nested_attributes_for',
  'default_scope', 'scope',
  -- ActiveSupport.
  'alias_method_chain', 'alias_attribute', 'delegate', 'cattr_accessor', 'mattr_accessor',
  'returning', 'memoize'
}) + lex:get_rule('function'))

return lex
