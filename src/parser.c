/*
 * Copyright 2016 Alexander Fedyashov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parser.h"
#include <regex.h>

zend_class_entry *gumbo_parser_class_entry;

//
// Argument declarations.
//

ZEND_BEGIN_ARG_INFO_EX(arginfo_gumbo_parser_load, 0, 0, 1)
    ZEND_ARG_INFO(0, html)
ZEND_END_ARG_INFO()

//

zend_function_entry gumbo_parser_methods[] = {
    PHP_ME(GumboParser, load, arginfo_gumbo_parser_load, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

static const char* gumbo_xmlns[] = {
  "http://www.w3.org/1999/xhtml",
  "http://www.w3.org/2000/svg",
  "http://www.w3.org/1998/Math/MathML"
};

//
// Main parser function. Parses HTML with Gumbo and runs recursive parsing for elements.
//
xmlDocPtr gumbo_parse_string(zval* html) {
    // Run Gumbo Parser.

    GumboOptions gumbo_options = kGumboDefaultOptions;
    GumboOutput* gumbo_output = gumbo_parse_with_options(&gumbo_options, Z_STRVAL_P(html), Z_STRLEN_P(html));

    // Create new xmlDoc.

    xmlDocPtr xml_doc = xmlNewDoc(BAD_CAST "1.0");
    GumboDocument* gumbo_document = &gumbo_output->document->v.document;

    // If HTML page has DTD (Document Type Definition) then add it to xmlDoc.
    
    if(gumbo_document->has_doctype) {
//        xmlDtdPtr doc_type = xmlNewDtd(
//            xml_doc,
//            BAD_CAST gumbo_document->name,
//            BAD_CAST gumbo_document->public_identifier,
//            BAD_CAST gumbo_document->system_identifier
//        );
//
//        xml_doc->intSubset = doc_type;
    }

    // Main parsing process.

    gumbo_recursive_parse(xml_doc, (xmlNodePtr)xml_doc, gumbo_output->root);

    // Clearing gumbo and returning xmlDoc.

    // TODO: Errors to libXML

    gumbo_destroy_output(&gumbo_options, gumbo_output);

    return xml_doc;
}

//
// Recursive parser function.
//
void gumbo_recursive_parse(xmlDocPtr doc, xmlNodePtr parent_node, GumboNode* node) {

    if(node->parse_flags & GUMBO_INSERTION_BY_PARSER) {
        gumbo_skip_element(doc, parent_node, node);

        return;
    }

    switch(node->type) {
        case GUMBO_NODE_DOCUMENT:
            // @TODO: Throw exception
        break;

        case GUMBO_NODE_ELEMENT:
        case GUMBO_NODE_TEMPLATE:
            gumbo_parse_element(doc, parent_node, node);
        break;

        case GUMBO_NODE_CDATA:
        case GUMBO_NODE_TEXT:
            xmlAddChild(parent_node, xmlNewText(gumbo_get_text(node)));
        break;

        case GUMBO_NODE_COMMENT:
            xmlAddChild(parent_node, xmlNewComment(BAD_CAST node->v.text.text));
        break;

        // TODO: Whitespace option
        // case GUMBO_NODE_WHITESPACE:
        //    xmlAddChild(parent_node, xmlNewText(BAD_CAST node->v.text.text));
    }
}

//
// Function for skipping elements that Gumbo generated automatically.
//
void gumbo_skip_element(xmlDocPtr doc, xmlNodePtr parent_node, GumboNode* node) {
    GumboVector children = node->v.element.children;

    // Skip processing if node doesn't have children.

    if(children.length == 0) {
        return;
    }

    // If node has only one child, loop isn't need.

    if(children.length == 1) {
        GumboNode* child_node = children.data[0];
        gumbo_recursive_parse(doc, parent_node, child_node);

        return;
    }

    // Run loop for children elements.

    for (int i = 0; i < children.length; i++) {
        GumboNode* child_node = children.data[i];
        gumbo_recursive_parse(doc, parent_node, child_node);
    }
}

//
// Function for processing elements.
//
void gumbo_parse_element(xmlDocPtr doc, xmlNodePtr parent_node, GumboNode* node) {
    GumboElement* element = &node->v.element;
    int i;

    // Creating XML node.

    xmlNodePtr result_node = xmlNewNode(NULL, gumbo_get_tag_name(element));

    // Processing namespaces.

    if (node->parent->type != GUMBO_NODE_DOCUMENT
        && element->tag_namespace != node->parent->v.element.tag_namespace
    ) {
        xmlNsPtr namespace = xmlNewNs(
            result_node,
            BAD_CAST gumbo_xmlns[element->tag_namespace],
            NULL
        );

        xmlSetNs(result_node, namespace);
    }

    // Processing attributes.

    GumboVector attributes = element->attributes;

    if(attributes.length > 0) {
        for (i = 0; i < attributes.length; ++i) {
            GumboAttribute* gumbo_attr = attributes.data[i];

            // Manual processing of attribute name.

            int name_length = gumbo_attr->original_name.length;
            char *attr_name = malloc(sizeof(char) * name_length + 1);

            strncpy(attr_name, gumbo_attr->original_name.data, name_length);
            attr_name[name_length] = '\0';

            xmlNewProp(result_node, BAD_CAST attr_name, BAD_CAST gumbo_attr->value);
            free(attr_name);
        }
    }

    // Processing children.

    GumboVector children = element->children;

    if(children.length > 0) {
        for (i = 0; i < children.length; ++i) {
            gumbo_recursive_parse(doc, result_node, children.data[i]);
        }
    }

    xmlAddChild(parent_node, result_node);
}

//
// Function that returns pointer to real tag name.
//
xmlChar* gumbo_get_tag_name(GumboElement* element) {
    GumboStringPiece* tag = &element->original_tag;
    gumbo_tag_from_original_text(tag);

    char* tag_name = malloc(sizeof(char) * tag->length + 1);

    strncpy(tag_name, tag->data, tag->length);
    tag_name[tag->length]= '\0';

    return BAD_CAST tag_name;
}

//
// Function that returns pointer to real text.
//
xmlChar* gumbo_get_text(GumboNode* node) {
    if (node->v.text.text[0] == '\0') {
       return BAD_CAST node->v.text.text;
    }

    GumboStringPiece* text = &node->v.text.original_text;
    int length = gumbo_get_text_length(node, text);
    char* real_text;

    real_text = malloc(sizeof(char) * length + 1);
    strncpy(real_text, text->data, length);
    real_text[length] = '\0';

    return BAD_CAST real_text;
}

int gumbo_get_text_length(GumboNode* node, GumboStringPiece* text) {
   GumboElement* parent_element = &node->parent->v.element;

   if(parent_element->tag != GUMBO_TAG_BODY) {
     return (int) text->length;
   }

   int reg_ret;
   regmatch_t reg_matches[1];
   regex_t reg_ex;

   reg_ret = regcomp(&reg_ex, "<([[:alpha:]]|/)", REG_EXTENDED);

   if (reg_ret) {
     // TODO: Throw fatal error
     exit(1);
   }

   reg_ret = regexec(&reg_ex, text->data, 1, reg_matches, 0);

   if (reg_ret == 0) {
 //printf("match (%d): %s \n", reg_matches[0].rm_so, text->data);

     return (int) reg_matches[0].rm_so;
   }

   if (reg_ret == REG_NOMATCH) {
   //printf("%s \n", text->data);
     regfree(&reg_ex);

     return (int) text->length;
   }

   regfree(&reg_ex);
   exit(1);
   // TODO: Throw fatal error
}

//
// Method for parsing HTML into DOMDocument.
// \DomDocument Layershifter\Gumbo::load(string $html);
//
PHP_METHOD(GumboParser, load) {
    zval *html;

    // Parsing input values

    #if PHP_MAJOR_VERSION < 7
        int result = zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &html);
    #else
        int result = zend_parse_parameters(ZEND_NUM_ARGS(), "z", &html);
    #endif

    if (result == FAILURE) {
        // @TODO: Throw exception

        RETURN_BOOL(false);
    }

    convert_to_string(html);

    // Parsing document and return DOMDocument

    int ret;
    xmlDocPtr doc = gumbo_parse_string(html);

    DOM_RET_OBJ((xmlNodePtr) doc, &ret, NULL);
}
